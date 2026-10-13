# Game clients put JSON telemetry records on the Kinesis data stream. Each
# record must contain string fields "source" (user | ai_train | ai_test |
# multiplayer), "owner_id" (the Cognito identity ID) and "game_id". Firehose
# partitions records by those fields so every game gets its own S3 prefix:
#   s3://<bucket>/telemetry/source=<source>/owner_id=<owner_id>/game_id=<game_id>/
# That prefix is what game records store as telemetry_s3_uri.

locals {
  telemetry_prefix = "telemetry/source=!{partitionKeyFromQuery:source}/owner_id=!{partitionKeyFromQuery:owner_id}/game_id=!{partitionKeyFromQuery:game_id}/"

  # Strip characters that could inject extra path segments into the S3 key.
  # Records missing a field fail the query and land under telemetry-errors/.
  telemetry_partition_query = join("", [
    "{",
    "source: (.source | gsub(\"[^A-Za-z0-9_-]\"; \"_\")), ",
    "owner_id: (.owner_id | gsub(\"[^A-Za-z0-9:_-]\"; \"_\")), ",
    "game_id: (.game_id | gsub(\"[^A-Za-z0-9_-]\"; \"_\"))",
    "}",
  ])
}

resource "aws_kinesis_stream" "telemetry" {
  name             = "${local.name_prefix}-telemetry"
  shard_count      = var.telemetry_stream_mode == "PROVISIONED" ? var.telemetry_shard_count : null
  retention_period = 24
  encryption_type  = "KMS"
  kms_key_id       = "alias/aws/kinesis"

  stream_mode_details {
    stream_mode = var.telemetry_stream_mode
  }
}

resource "aws_s3_bucket" "telemetry" {
  bucket        = "${local.name_prefix}-telemetry-${local.bucket_suffix}"
  force_destroy = !var.deletion_protection
}

resource "aws_s3_bucket_ownership_controls" "telemetry" {
  bucket = aws_s3_bucket.telemetry.id

  rule {
    object_ownership = "BucketOwnerEnforced"
  }
}

resource "aws_s3_bucket_public_access_block" "telemetry" {
  bucket = aws_s3_bucket.telemetry.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

resource "aws_s3_bucket_server_side_encryption_configuration" "telemetry" {
  bucket = aws_s3_bucket.telemetry.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_lifecycle_configuration" "telemetry" {
  bucket = aws_s3_bucket.telemetry.id

  rule {
    id     = "telemetry-tiering"
    status = "Enabled"

    filter {}

    transition {
      days          = var.telemetry_infrequent_access_days
      storage_class = "STANDARD_IA"
    }

    dynamic "expiration" {
      for_each = var.telemetry_expiration_days > 0 ? [var.telemetry_expiration_days] : []

      content {
        days = expiration.value
      }
    }

    abort_incomplete_multipart_upload {
      days_after_initiation = 7
    }
  }
}

data "aws_iam_policy_document" "telemetry_bucket" {
  statement {
    sid     = "DenyInsecureTransport"
    effect  = "Deny"
    actions = ["s3:*"]
    resources = [
      aws_s3_bucket.telemetry.arn,
      "${aws_s3_bucket.telemetry.arn}/*",
    ]

    principals {
      type        = "*"
      identifiers = ["*"]
    }

    condition {
      test     = "Bool"
      variable = "aws:SecureTransport"
      values   = ["false"]
    }
  }
}

resource "aws_s3_bucket_policy" "telemetry" {
  bucket = aws_s3_bucket.telemetry.id
  policy = data.aws_iam_policy_document.telemetry_bucket.json

  depends_on = [aws_s3_bucket_public_access_block.telemetry]
}

resource "aws_cloudwatch_log_group" "telemetry_firehose" {
  name              = "/aws/kinesisfirehose/${local.name_prefix}-telemetry"
  retention_in_days = 30
}

resource "aws_cloudwatch_log_stream" "telemetry_firehose" {
  name           = "S3Delivery"
  log_group_name = aws_cloudwatch_log_group.telemetry_firehose.name
}

data "aws_iam_policy_document" "firehose_assume_role" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["firehose.amazonaws.com"]
    }

    condition {
      test     = "StringEquals"
      variable = "sts:ExternalId"
      values   = [local.account_id]
    }
  }
}

resource "aws_iam_role" "telemetry_firehose" {
  name               = "${local.name_prefix}-telemetry-firehose"
  assume_role_policy = data.aws_iam_policy_document.firehose_assume_role.json
}

data "aws_iam_policy_document" "telemetry_firehose" {
  statement {
    sid = "ReadTelemetryStream"
    actions = [
      "kinesis:DescribeStream",
      "kinesis:DescribeStreamSummary",
      "kinesis:GetShardIterator",
      "kinesis:GetRecords",
      "kinesis:ListShards",
    ]
    resources = [aws_kinesis_stream.telemetry.arn]
  }

  statement {
    sid = "WriteTelemetryBucket"
    actions = [
      "s3:AbortMultipartUpload",
      "s3:GetBucketLocation",
      "s3:GetObject",
      "s3:ListBucket",
      "s3:ListBucketMultipartUploads",
      "s3:PutObject",
    ]
    resources = [
      aws_s3_bucket.telemetry.arn,
      "${aws_s3_bucket.telemetry.arn}/*",
    ]
  }

  statement {
    sid       = "WriteDeliveryLogs"
    actions   = ["logs:PutLogEvents"]
    resources = ["${aws_cloudwatch_log_group.telemetry_firehose.arn}:*"]
  }
}

resource "aws_iam_role_policy" "telemetry_firehose" {
  name   = "telemetry-delivery"
  role   = aws_iam_role.telemetry_firehose.id
  policy = data.aws_iam_policy_document.telemetry_firehose.json
}

resource "aws_kinesis_firehose_delivery_stream" "telemetry" {
  name        = "${local.name_prefix}-telemetry-to-s3"
  destination = "extended_s3"

  kinesis_source_configuration {
    kinesis_stream_arn = aws_kinesis_stream.telemetry.arn
    role_arn           = aws_iam_role.telemetry_firehose.arn
  }

  extended_s3_configuration {
    role_arn            = aws_iam_role.telemetry_firehose.arn
    bucket_arn          = aws_s3_bucket.telemetry.arn
    prefix              = local.telemetry_prefix
    error_output_prefix = "telemetry-errors/!{firehose:error-output-type}/!{timestamp:yyyy/MM/dd}/"
    compression_format  = "GZIP"

    # Dynamic partitioning requires a buffer of at least 64 MiB.
    buffering_size     = 64
    buffering_interval = var.telemetry_buffer_interval_seconds

    dynamic_partitioning_configuration {
      enabled = true
    }

    processing_configuration {
      enabled = true

      processors {
        type = "MetadataExtraction"

        parameters {
          parameter_name  = "JsonParsingEngine"
          parameter_value = "JQ-1.6"
        }

        parameters {
          parameter_name  = "MetadataExtractionQuery"
          parameter_value = local.telemetry_partition_query
        }
      }

      processors {
        type = "AppendDelimiterToRecord"
      }
    }

    cloudwatch_logging_options {
      enabled         = true
      log_group_name  = aws_cloudwatch_log_group.telemetry_firehose.name
      log_stream_name = aws_cloudwatch_log_stream.telemetry_firehose.name
    }
  }

  depends_on = [aws_iam_role_policy.telemetry_firehose]
}

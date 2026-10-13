# Multiplayer is still in development. Matchmaking, the game session queue and
# the build bucket are always created; the server build and fleet are created
# only once var.gamelift_server_build points at an uploaded server build.

locals {
  gamelift_fleet_enabled = var.gamelift_server_build != null

  matchmaking_config_name = "${local.name_prefix}-free-for-all"
  rule_set_body           = file("${path.module}/matchmaking/rule_set.json")
  # Rule sets are immutable, so a content hash in the name lets a changed rule
  # set be created before the old one (still referenced) is deleted.
  rule_set_name = "${local.name_prefix}-free-for-all-${substr(sha1(local.rule_set_body), 0, 8)}"

  awscc_tags = [for key, value in local.common_tags : { key = key, value = value }]
}

# ---------------------------------------------------------------------------
# Server builds
# ---------------------------------------------------------------------------

resource "aws_s3_bucket" "gamelift_builds" {
  bucket        = "${local.name_prefix}-gamelift-builds-${local.bucket_suffix}"
  force_destroy = !var.deletion_protection
}

resource "aws_s3_bucket_ownership_controls" "gamelift_builds" {
  bucket = aws_s3_bucket.gamelift_builds.id

  rule {
    object_ownership = "BucketOwnerEnforced"
  }
}

resource "aws_s3_bucket_public_access_block" "gamelift_builds" {
  bucket = aws_s3_bucket.gamelift_builds.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

resource "aws_s3_bucket_versioning" "gamelift_builds" {
  bucket = aws_s3_bucket.gamelift_builds.id

  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "gamelift_builds" {
  bucket = aws_s3_bucket.gamelift_builds.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

data "aws_iam_policy_document" "gamelift_builds_bucket" {
  statement {
    sid     = "DenyInsecureTransport"
    effect  = "Deny"
    actions = ["s3:*"]
    resources = [
      aws_s3_bucket.gamelift_builds.arn,
      "${aws_s3_bucket.gamelift_builds.arn}/*",
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

resource "aws_s3_bucket_policy" "gamelift_builds" {
  bucket = aws_s3_bucket.gamelift_builds.id
  policy = data.aws_iam_policy_document.gamelift_builds_bucket.json

  depends_on = [aws_s3_bucket_public_access_block.gamelift_builds]
}

data "aws_iam_policy_document" "gamelift_assume_role" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["gamelift.amazonaws.com"]
    }

    condition {
      test     = "StringEquals"
      variable = "aws:SourceAccount"
      values   = [local.account_id]
    }
  }
}

# Lets GameLift Servers read uploaded builds.
resource "aws_iam_role" "gamelift_build_access" {
  name               = "${local.name_prefix}-gamelift-build-access"
  assume_role_policy = data.aws_iam_policy_document.gamelift_assume_role.json
}

data "aws_iam_policy_document" "gamelift_build_access" {
  statement {
    actions   = ["s3:GetObject", "s3:GetObjectVersion"]
    resources = ["${aws_s3_bucket.gamelift_builds.arn}/*"]
  }
}

resource "aws_iam_role_policy" "gamelift_build_access" {
  name   = "read-builds"
  role   = aws_iam_role.gamelift_build_access.id
  policy = data.aws_iam_policy_document.gamelift_build_access.json
}

resource "aws_gamelift_build" "server" {
  count = local.gamelift_fleet_enabled ? 1 : 0

  name             = "${local.name_prefix}-server"
  version          = var.gamelift_server_build.version
  operating_system = var.gamelift_server_build.operating_system

  storage_location {
    bucket   = aws_s3_bucket.gamelift_builds.id
    key      = var.gamelift_server_build.s3_key
    role_arn = aws_iam_role.gamelift_build_access.arn
  }

  lifecycle {
    create_before_destroy = true
  }

  depends_on = [aws_iam_role_policy.gamelift_build_access]
}

# ---------------------------------------------------------------------------
# Fleet
# ---------------------------------------------------------------------------

# Credentials available to game server processes on fleet instances.
resource "aws_iam_role" "gamelift_fleet_instance" {
  name               = "${local.name_prefix}-gamelift-fleet-instance"
  assume_role_policy = data.aws_iam_policy_document.gamelift_assume_role.json
}

data "aws_iam_policy_document" "gamelift_fleet_instance" {
  statement {
    sid       = "PublishMatchTelemetry"
    actions   = ["kinesis:PutRecord", "kinesis:PutRecords"]
    resources = [aws_kinesis_stream.telemetry.arn]
  }
}

resource "aws_iam_role_policy" "gamelift_fleet_instance" {
  name   = "game-server"
  role   = aws_iam_role.gamelift_fleet_instance.id
  policy = data.aws_iam_policy_document.gamelift_fleet_instance.json
}

resource "aws_gamelift_fleet" "server" {
  count = local.gamelift_fleet_enabled ? 1 : 0

  name                               = "${local.name_prefix}-server-${var.gamelift_server_build.version}"
  build_id                           = aws_gamelift_build.server[0].id
  ec2_instance_type                  = var.gamelift_server_build.instance_type
  fleet_type                         = var.gamelift_server_build.fleet_type
  instance_role_arn                  = aws_iam_role.gamelift_fleet_instance.arn
  new_game_session_protection_policy = "FullProtection"
  metric_groups                      = [local.name_prefix]

  dynamic "ec2_inbound_permission" {
    for_each = ["UDP", "TCP"]

    content {
      from_port = var.gamelift_server_port_range.from
      to_port   = var.gamelift_server_port_range.to
      ip_range  = "0.0.0.0/0"
      protocol  = ec2_inbound_permission.value
    }
  }

  runtime_configuration {
    game_session_activation_timeout_seconds = 300
    max_concurrent_game_session_activations = 2

    server_process {
      concurrent_executions = var.gamelift_server_build.processes
      launch_path           = var.gamelift_server_build.launch_path
      parameters            = var.gamelift_server_build.launch_params
    }
  }

  resource_creation_limit_policy {
    new_game_sessions_per_creator = 3
    policy_period_in_minutes      = 15
  }

  lifecycle {
    create_before_destroy = true
  }

  depends_on = [aws_iam_role_policy.gamelift_fleet_instance]
}

# Stable target for the queue; points at the current fleet, or rejects
# placements with a message while no fleet exists.
resource "aws_gamelift_alias" "server" {
  name        = "${local.name_prefix}-server"
  description = "Current Snake multiplayer server fleet."

  routing_strategy {
    type     = local.gamelift_fleet_enabled ? "SIMPLE" : "TERMINAL"
    fleet_id = local.gamelift_fleet_enabled ? aws_gamelift_fleet.server[0].id : null
    message  = local.gamelift_fleet_enabled ? null : "Snake multiplayer is not available yet."
  }
}

# ---------------------------------------------------------------------------
# Game session placement and FlexMatch matchmaking
# ---------------------------------------------------------------------------

resource "aws_sns_topic" "matchmaking_events" {
  name = "${local.name_prefix}-matchmaking-events"
}

data "aws_iam_policy_document" "matchmaking_events" {
  statement {
    sid       = "AllowGameLiftPublish"
    actions   = ["sns:Publish"]
    resources = [aws_sns_topic.matchmaking_events.arn]

    principals {
      type        = "Service"
      identifiers = ["gamelift.amazonaws.com"]
    }

    condition {
      test     = "StringEquals"
      variable = "aws:SourceAccount"
      values   = [local.account_id]
    }

    # AWS documents matchmaking configuration ARNs both with and without an
    # account ID, so wildcard that segment; aws:SourceAccount pins the account.
    condition {
      test     = "ArnLike"
      variable = "aws:SourceArn"
      values   = ["arn:${local.partition}:gamelift:${var.aws_region}:*:matchmakingconfiguration/${local.matchmaking_config_name}"]
    }
  }
}

resource "aws_sns_topic_policy" "matchmaking_events" {
  arn    = aws_sns_topic.matchmaking_events.arn
  policy = data.aws_iam_policy_document.matchmaking_events.json
}

resource "aws_gamelift_game_session_queue" "multiplayer" {
  name               = "${local.name_prefix}-multiplayer"
  timeout_in_seconds = 120
  destinations       = local.gamelift_fleet_enabled ? [aws_gamelift_alias.server.arn] : []
}

resource "awscc_gamelift_matchmaking_rule_set" "free_for_all" {
  name          = local.rule_set_name
  rule_set_body = local.rule_set_body
  tags          = local.awscc_tags

  lifecycle {
    create_before_destroy = true
  }
}

resource "awscc_gamelift_matchmaking_configuration" "free_for_all" {
  name                    = local.matchmaking_config_name
  description             = "Free-for-all Snake matches for 2-4 players."
  flex_match_mode         = "WITH_QUEUE"
  game_session_queue_arns = [aws_gamelift_game_session_queue.multiplayer.arn]
  rule_set_name           = awscc_gamelift_matchmaking_rule_set.free_for_all.name
  request_timeout_seconds = var.matchmaking_request_timeout_seconds
  acceptance_required     = false
  backfill_mode           = "MANUAL"
  additional_player_count = 0
  notification_target     = aws_sns_topic.matchmaking_events.arn
  tags                    = local.awscc_tags

  depends_on = [aws_sns_topic_policy.matchmaking_events]
}

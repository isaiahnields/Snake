# One-time setup applied locally with an administrator's credentials. Creates
# the remote state bucket and the roles GitHub Actions assumes through OIDC.

data "aws_caller_identity" "current" {}

data "aws_partition" "current" {}

locals {
  account_id = data.aws_caller_identity.current.account_id
  partition  = data.aws_partition.current.partition
  prefix     = "${var.project}-"
  oidc_host  = "token.actions.githubusercontent.com"
  oidc_arn = var.create_github_oidc_provider ? aws_iam_openid_connect_provider.github[0].arn : (
    "arn:${local.partition}:iam::${local.account_id}:oidc-provider/${local.oidc_host}"
  )
}

# ---------------------------------------------------------------------------
# Remote state
# ---------------------------------------------------------------------------

resource "aws_s3_bucket" "state" {
  bucket = "${var.project}-terraform-state-${local.account_id}-${var.aws_region}"

  lifecycle {
    prevent_destroy = true
  }
}

resource "aws_s3_bucket_ownership_controls" "state" {
  bucket = aws_s3_bucket.state.id

  rule {
    object_ownership = "BucketOwnerEnforced"
  }
}

resource "aws_s3_bucket_public_access_block" "state" {
  bucket = aws_s3_bucket.state.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

resource "aws_s3_bucket_versioning" "state" {
  bucket = aws_s3_bucket.state.id

  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "state" {
  bucket = aws_s3_bucket.state.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_lifecycle_configuration" "state" {
  bucket = aws_s3_bucket.state.id

  rule {
    id     = "expire-old-state-versions"
    status = "Enabled"

    filter {}

    noncurrent_version_expiration {
      noncurrent_days           = 90
      newer_noncurrent_versions = 10
    }
  }
}

data "aws_iam_policy_document" "state_bucket" {
  statement {
    sid     = "DenyInsecureTransport"
    effect  = "Deny"
    actions = ["s3:*"]
    resources = [
      aws_s3_bucket.state.arn,
      "${aws_s3_bucket.state.arn}/*",
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

resource "aws_s3_bucket_policy" "state" {
  bucket = aws_s3_bucket.state.id
  policy = data.aws_iam_policy_document.state_bucket.json

  depends_on = [aws_s3_bucket_public_access_block.state]
}

# ---------------------------------------------------------------------------
# GitHub Actions OIDC
# ---------------------------------------------------------------------------

resource "aws_iam_openid_connect_provider" "github" {
  count = var.create_github_oidc_provider ? 1 : 0

  url            = "https://${local.oidc_host}"
  client_id_list = ["sts.amazonaws.com"]
}

data "aws_iam_policy_document" "state_read" {
  statement {
    actions   = ["s3:ListBucket"]
    resources = [aws_s3_bucket.state.arn]
  }

  statement {
    actions   = ["s3:GetObject"]
    resources = ["${aws_s3_bucket.state.arn}/*"]
  }
}

data "aws_iam_policy_document" "state_access" {
  statement {
    actions   = ["s3:ListBucket"]
    resources = [aws_s3_bucket.state.arn]
  }

  # Read/write includes the .tflock lock objects used by use_lockfile.
  statement {
    actions   = ["s3:GetObject", "s3:PutObject", "s3:DeleteObject"]
    resources = ["${aws_s3_bucket.state.arn}/*"]
  }
}

# Plan role: pull requests and pushes to the default branch. Read-only,
# including state (CI plans run with -lock=false).
data "aws_iam_policy_document" "plan_trust" {
  statement {
    actions = ["sts:AssumeRoleWithWebIdentity"]

    principals {
      type        = "Federated"
      identifiers = [local.oidc_arn]
    }

    condition {
      test     = "StringEquals"
      variable = "${local.oidc_host}:aud"
      values   = ["sts.amazonaws.com"]
    }

    condition {
      test     = "StringEquals"
      variable = "${local.oidc_host}:sub"
      values = ["repo:isaiahnields@12752162/snake@1413030131:ref:refs/head/main"]
    }
  }
}

resource "aws_iam_role" "plan" {
  name                 = "${var.project}-terraform-plan"
  description          = "Assumed by GitHub Actions to run terraform plan."
  assume_role_policy   = data.aws_iam_policy_document.plan_trust.json
  max_session_duration = 3600
}

resource "aws_iam_role_policy" "plan_assume_role_with_web_identity" {
  name = "assume-role-with-web-identity"
  role = aws_iam_role.plan.id
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect   = "Allow"
      Action   = "sts:AssumeRoleWithWebIdentity"
      Resource = "*"
    }]
  })
}

resource "aws_iam_role_policy_attachment" "plan_read_only" {
  role       = aws_iam_role.plan.name
  policy_arn = "arn:${local.partition}:iam::aws:policy/ReadOnlyAccess"
}

resource "aws_iam_role_policy" "plan_state" {
  name   = "terraform-state-read"
  role   = aws_iam_role.plan.id
  policy = data.aws_iam_policy_document.state_read.json
}

# Apply role: only jobs running in the protected GitHub environment.
data "aws_iam_policy_document" "apply_trust" {
  statement {
    actions = ["sts:AssumeRoleWithWebIdentity"]

    principals {
      type        = "Federated"
      identifiers = [local.oidc_arn]
    }

    condition {
      test     = "StringEquals"
      variable = "${local.oidc_host}:aud"
      values   = ["sts.amazonaws.com"]
    }

    condition {
      test     = "StringEquals"
      variable = "${local.oidc_host}:sub"
      values   = ["repo:isaiahnields@12752162/snake@1413030131:ref:refs/head/main"]
    }
  }
}

resource "aws_iam_role" "apply" {
  name                 = "${var.project}-terraform-apply"
  description          = "Assumed by GitHub Actions to run terraform apply."
  assume_role_policy   = data.aws_iam_policy_document.apply_trust.json
  max_session_duration = 7200
}

resource "aws_iam_role_policy" "apply_assume_role_with_web_identity" {
  name = "assume-role-with-web-identity"
  role = aws_iam_role.apply.id
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect   = "Allow"
      Action   = "sts:AssumeRoleWithWebIdentity"
      Resource = "*"
    }]
  })
}

resource "aws_iam_role_policy_attachment" "apply_read_only" {
  role       = aws_iam_role.apply.name
  policy_arn = "arn:${local.partition}:iam::aws:policy/ReadOnlyAccess"
}

resource "aws_iam_role_policy" "apply_state" {
  name   = "terraform-state"
  role   = aws_iam_role.apply.id
  policy = data.aws_iam_policy_document.state_access.json
}

# Write access to the services used by infra/terraform. Resources with
# user-chosen names are limited to the project prefix; services whose
# resources are addressed by generated IDs are granted account-wide.
data "aws_iam_policy_document" "apply_manage" {
  statement {
    sid     = "PrefixedResources"
    actions = ["dynamodb:*", "kinesis:*", "firehose:*", "lambda:*", "sns:*", "s3:*"]
    resources = [
      "arn:${local.partition}:dynamodb:${var.aws_region}:${local.account_id}:table/${local.prefix}*",
      "arn:${local.partition}:kinesis:${var.aws_region}:${local.account_id}:stream/${local.prefix}*",
      "arn:${local.partition}:firehose:${var.aws_region}:${local.account_id}:deliverystream/${local.prefix}*",
      "arn:${local.partition}:lambda:${var.aws_region}:${local.account_id}:function:${local.prefix}*",
      "arn:${local.partition}:sns:${var.aws_region}:${local.account_id}:${local.prefix}*",
      "arn:${local.partition}:s3:::${local.prefix}*",
    ]
  }

  statement {
    sid     = "PrefixedLogGroups"
    actions = ["logs:*"]
    resources = [
      "arn:${local.partition}:logs:${var.aws_region}:${local.account_id}:log-group:/aws/lambda/${local.prefix}*",
      "arn:${local.partition}:logs:${var.aws_region}:${local.account_id}:log-group:/aws/kinesisfirehose/${local.prefix}*",
    ]
  }

  statement {
    sid       = "PrefixedRoles"
    actions   = ["iam:*"]
    resources = ["arn:${local.partition}:iam::${local.account_id}:role/${local.prefix}*"]
  }

  statement {
    sid       = "ServiceLinkedRoles"
    actions   = ["iam:CreateServiceLinkedRole"]
    resources = ["*"]

    condition {
      test     = "StringEquals"
      variable = "iam:AWSServiceName"
      values = [
        "gamelift.amazonaws.com",
        "firehose.amazonaws.com",
        "ops.apigateway.amazonaws.com",
      ]
    }
  }

  statement {
    sid = "IdAddressedServices"
    actions = [
      "apigateway:*",
      "cognito-identity:*",
      "cognito-idp:*",
      "gamelift:*",
      "logs:CreateLogDelivery",
      "logs:DeleteLogDelivery",
      "logs:GetLogDelivery",
      "logs:UpdateLogDelivery",
      "logs:ListLogDeliveries",
    ]
    resources = ["*"]
  }

  statement {
    sid = "CloudControl"
    actions = [
      "cloudformation:CreateResource",
      "cloudformation:DeleteResource",
      "cloudformation:GetResource",
      "cloudformation:GetResourceRequestStatus",
      "cloudformation:ListResources",
      "cloudformation:UpdateResource",
    ]
    resources = ["*"]
  }

  # Guardrails: the pipeline must not change its own roles or the state bucket.
  statement {
    sid     = "ProtectBootstrapRoles"
    effect  = "Deny"
    actions = ["iam:*"]
    resources = [
      aws_iam_role.plan.arn,
      aws_iam_role.apply.arn,
    ]
  }

  statement {
    sid    = "ProtectStateBucket"
    effect = "Deny"
    actions = [
      "s3:DeleteBucket",
      "s3:DeleteBucketPolicy",
      "s3:PutBucketPolicy",
      "s3:PutBucketVersioning",
      "s3:PutLifecycleConfiguration",
      "s3:PutBucketPublicAccessBlock",
    ]
    resources = [aws_s3_bucket.state.arn]
  }

  statement {
    sid       = "KinesisManagedKey"
    actions   = ["kms:DescribeKey", "kms:GenerateDataKey*", "kms:Decrypt", "kms:Encrypt", "kms:CreateGrant"]
    resources = ["*"]

    condition {
      test     = "StringEquals"
      variable = "kms:ViaService"
      values   = ["kinesis.${var.aws_region}.amazonaws.com"]
    }
  }
}

resource "aws_iam_role_policy" "apply_manage" {
  name   = "manage-snake-backend"
  role   = aws_iam_role.apply.id
  policy = data.aws_iam_policy_document.apply_manage.json
}

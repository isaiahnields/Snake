data "aws_caller_identity" "current" {}

data "aws_partition" "current" {}

locals {
  name_prefix = "${var.project}-${var.environment}"
  account_id  = data.aws_caller_identity.current.account_id
  partition   = data.aws_partition.current.partition

  # Bucket names must be globally unique.
  bucket_suffix = "${local.account_id}-${var.aws_region}"

  common_tags = {
    Project     = var.project
    Environment = var.environment
    ManagedBy   = "terraform"
    Repository  = "isaiahnields/Snake"
  }

  # IAM policy variable resolving to the caller's Cognito identity ID. Every
  # player-owned DynamoDB item and S3 telemetry prefix is keyed by this value.
  cognito_identity_id = "$${cognito-identity.amazonaws.com:sub}"
}

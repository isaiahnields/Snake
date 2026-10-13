output "state_bucket" {
  description = "Set as the TF_STATE_BUCKET GitHub Actions variable."
  value       = aws_s3_bucket.state.id
}

output "aws_region" {
  description = "Set as the AWS_REGION GitHub Actions variable."
  value       = var.aws_region
}

output "plan_role_arn" {
  description = "Set as the AWS_PLAN_ROLE_ARN GitHub Actions variable."
  value       = aws_iam_role.plan.arn
}

output "apply_role_arn" {
  description = "Set as the AWS_APPLY_ROLE_ARN GitHub Actions variable."
  value       = aws_iam_role.apply.arn
}

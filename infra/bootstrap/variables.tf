variable "aws_region" {
  description = "Region for the Terraform state bucket. Use the same region as the main stack."
  type        = string
  default     = "us-west-2"
}

variable "project" {
  description = "Project prefix. Must match var.project in infra/terraform; the apply role may only manage resources with this prefix."
  type        = string
  default     = "snake"
}

variable "github_repository" {
  description = "GitHub repository (owner/name) allowed to assume the Terraform roles."
  type        = string
  default     = "isaiahnields/snake"
}

variable "github_default_branch" {
  description = "Branch whose pushes may run terraform plan."
  type        = string
  default     = "main"
}

variable "github_apply_environment" {
  description = "GitHub Actions environment whose jobs may assume the apply role."
  type        = string
  default     = "production"
}

variable "create_github_oidc_provider" {
  description = "Create the GitHub Actions OIDC provider. Set to false if the account already has one."
  type        = bool
  default     = true
}

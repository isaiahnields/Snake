terraform {
  required_version = ">= 1.10.0"

  required_providers {
    archive = {
      source  = "hashicorp/archive"
      version = "~> 2.8"
    }
    aws = {
      source  = "hashicorp/aws"
      version = "~> 6.68"
    }
    # FlexMatch matchmaking resources are only available through Cloud Control.
    awscc = {
      source  = "hashicorp/awscc"
      version = "~> 1.105"
    }
  }

  # Supply bucket and region with -backend-config (see infra/README.md).
  backend "s3" {
    key          = "snake/terraform.tfstate"
    encrypt      = true
    use_lockfile = true
  }
}

provider "aws" {
  region = var.aws_region

  default_tags {
    tags = local.common_tags
  }
}

provider "awscc" {
  region = var.aws_region
}

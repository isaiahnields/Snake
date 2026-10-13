resource "aws_cognito_user_pool" "players" {
  name                = "${local.name_prefix}-players"
  deletion_protection = var.deletion_protection ? "ACTIVE" : "INACTIVE"

  username_attributes      = ["email"]
  auto_verified_attributes = ["email"]
  mfa_configuration        = "OPTIONAL"

  username_configuration {
    case_sensitive = false
  }

  software_token_mfa_configuration {
    enabled = true
  }

  password_policy {
    minimum_length                   = 12
    require_lowercase                = true
    require_uppercase                = true
    require_numbers                  = true
    require_symbols                  = false
    temporary_password_validity_days = 7
  }

  account_recovery_setting {
    recovery_mechanism {
      name     = "verified_email"
      priority = 1
    }
  }

  admin_create_user_config {
    allow_admin_create_user_only = false
  }

  schema {
    name                = "email"
    attribute_data_type = "String"
    required            = true
    mutable             = true

    string_attribute_constraints {
      min_length = 3
      max_length = 254
    }
  }
}

# Public client used by the desktop and mobile game builds.
resource "aws_cognito_user_pool_client" "game" {
  name         = "${local.name_prefix}-game-client"
  user_pool_id = aws_cognito_user_pool.players.id

  generate_secret               = false
  prevent_user_existence_errors = "ENABLED"
  enable_token_revocation       = true

  explicit_auth_flows = [
    "ALLOW_USER_SRP_AUTH",
    "ALLOW_REFRESH_TOKEN_AUTH",
  ]

  access_token_validity  = 1
  id_token_validity      = 1
  refresh_token_validity = 30

  token_validity_units {
    access_token  = "hours"
    id_token      = "hours"
    refresh_token = "days"
  }
}

resource "aws_cognito_identity_pool" "players" {
  identity_pool_name               = "${local.name_prefix}-players"
  allow_unauthenticated_identities = false
  allow_classic_flow               = false

  cognito_identity_providers {
    client_id               = aws_cognito_user_pool_client.game.id
    provider_name           = aws_cognito_user_pool.players.endpoint
    server_side_token_check = true
  }
}

data "aws_iam_policy_document" "player_assume_role" {
  statement {
    actions = ["sts:AssumeRoleWithWebIdentity"]

    principals {
      type        = "Federated"
      identifiers = ["cognito-identity.amazonaws.com"]
    }

    condition {
      test     = "StringEquals"
      variable = "cognito-identity.amazonaws.com:aud"
      values   = [aws_cognito_identity_pool.players.id]
    }

    condition {
      test     = "ForAnyValue:StringLike"
      variable = "cognito-identity.amazonaws.com:amr"
      values   = ["authenticated"]
    }
  }
}

resource "aws_iam_role" "player" {
  name               = "${local.name_prefix}-player"
  description        = "Assumed by signed-in players through the Cognito identity pool."
  assume_role_policy = data.aws_iam_policy_document.player_assume_role.json
}

resource "aws_cognito_identity_pool_roles_attachment" "players" {
  identity_pool_id = aws_cognito_identity_pool.players.id

  roles = {
    authenticated = aws_iam_role.player.arn
  }
}

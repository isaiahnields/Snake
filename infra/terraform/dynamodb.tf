# All player-owned tables use the caller's Cognito identity ID as the partition
# key so IAM (dynamodb:LeadingKeys) confines each player to their own items.

# Game settings for manual play. One item per player; always editable.
#   owner_id (S, PK)  grid_width, grid_height, wall_collision, apple_spawn, ...
resource "aws_dynamodb_table" "user_settings" {
  name                        = "${local.name_prefix}-user-settings"
  billing_mode                = "PAY_PER_REQUEST"
  hash_key                    = "owner_id"
  deletion_protection_enabled = var.deletion_protection

  attribute {
    name = "owner_id"
    type = "S"
  }

  point_in_time_recovery {
    enabled = true
  }

  server_side_encryption {
    enabled = true
  }
}

# AI players, their model settings and the game settings they play on.
#   owner_id (S, PK), ai_player_id (S, SK), display_name, model_settings (M),
#   game_settings (M), created_at, updated_at
# Players only read this table. Items are written by the AI players API, which
# sets the settings once at creation and afterwards only changes display_name.
resource "aws_dynamodb_table" "ai_player" {
  name                        = "${local.name_prefix}-ai-player"
  billing_mode                = "PAY_PER_REQUEST"
  hash_key                    = "owner_id"
  range_key                   = "ai_player_id"
  deletion_protection_enabled = var.deletion_protection

  attribute {
    name = "owner_id"
    type = "S"
  }

  attribute {
    name = "ai_player_id"
    type = "S"
  }

  point_in_time_recovery {
    enabled = true
  }

  server_side_encryption {
    enabled = true
  }
}

# Manually played games.
#   owner_id (S, PK), game_id (S, SK; time-sortable such as a ULID), score (N),
#   game_settings (M), started_at, ended_at, telemetry_s3_uri
resource "aws_dynamodb_table" "user_game" {
  name                        = "${local.name_prefix}-user-game"
  billing_mode                = "PAY_PER_REQUEST"
  hash_key                    = "owner_id"
  range_key                   = "game_id"
  deletion_protection_enabled = var.deletion_protection

  attribute {
    name = "owner_id"
    type = "S"
  }

  attribute {
    name = "game_id"
    type = "S"
  }

  point_in_time_recovery {
    enabled = true
  }

  server_side_encryption {
    enabled = true
  }
}

# Games played by AI players during training and testing.
#   owner_id (S, PK), game_id (S, SK) = "<ai_player_id>#<train|test>#<game_id>",
#   ai_player_id, phase (train|test), episode (N), score (N), telemetry_s3_uri
# Query with begins_with(game_id, "<ai_player_id>#test#") to list one phase.
resource "aws_dynamodb_table" "ai_game" {
  name                        = "${local.name_prefix}-ai-game"
  billing_mode                = "PAY_PER_REQUEST"
  hash_key                    = "owner_id"
  range_key                   = "game_id"
  deletion_protection_enabled = var.deletion_protection

  attribute {
    name = "owner_id"
    type = "S"
  }

  attribute {
    name = "game_id"
    type = "S"
  }

  point_in_time_recovery {
    enabled = true
  }

  server_side_encryption {
    enabled = true
  }
}

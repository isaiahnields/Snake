# Permissions for signed-in players (Cognito identity pool authenticated role).
# dynamodb:LeadingKeys pins every request to items whose partition key equals
# the caller's Cognito identity ID. Scan is intentionally not granted.

data "aws_iam_policy_document" "player" {
  statement {
    sid = "UserPlaySettingsReadWrite"
    actions = [
      "dynamodb:GetItem",
      "dynamodb:PutItem",
      "dynamodb:UpdateItem",
      "dynamodb:DeleteItem",
    ]
    resources = [aws_dynamodb_table.user_play_settings.arn]

    condition {
      test     = "ForAllValues:StringEquals"
      variable = "dynamodb:LeadingKeys"
      values   = [local.cognito_identity_id]
    }
  }

  # Read-only: AI players are created and renamed through the AI players API so
  # their settings stay immutable.
  statement {
    sid = "AiPlayersRead"
    actions = [
      "dynamodb:GetItem",
      "dynamodb:BatchGetItem",
      "dynamodb:Query",
    ]
    resources = [aws_dynamodb_table.ai_players.arn]

    condition {
      test     = "ForAllValues:StringEquals"
      variable = "dynamodb:LeadingKeys"
      values   = [local.cognito_identity_id]
    }
  }

  statement {
    sid = "GameRecordsReadAndAppend"
    actions = [
      "dynamodb:GetItem",
      "dynamodb:BatchGetItem",
      "dynamodb:Query",
      "dynamodb:PutItem",
    ]
    resources = [
      aws_dynamodb_table.user_games.arn,
      aws_dynamodb_table.ai_games.arn,
    ]

    condition {
      test     = "ForAllValues:StringEquals"
      variable = "dynamodb:LeadingKeys"
      values   = [local.cognito_identity_id]
    }
  }

  statement {
    sid     = "AiPlayersApi"
    actions = ["execute-api:Invoke"]
    resources = [
      "${aws_api_gateway_stage.game.execution_arn}/POST/ai-players",
      "${aws_api_gateway_stage.game.execution_arn}/PATCH/ai-players/*",
    ]
  }

  statement {
    sid       = "PublishTelemetry"
    actions   = ["kinesis:PutRecord", "kinesis:PutRecords"]
    resources = [aws_kinesis_stream.telemetry.arn]
  }

  # Telemetry partition values are sanitized by Firehose so "/" and "=" cannot
  # appear inside them, which keeps these wildcards anchored to the real
  # owner_id path segment.
  statement {
    sid       = "ListOwnTelemetry"
    actions   = ["s3:ListBucket"]
    resources = [aws_s3_bucket.telemetry.arn]

    condition {
      test     = "StringLike"
      variable = "s3:prefix"
      values   = ["telemetry/*/owner_id=${local.cognito_identity_id}/*"]
    }
  }

  statement {
    sid       = "ReadOwnTelemetry"
    actions   = ["s3:GetObject"]
    resources = ["${aws_s3_bucket.telemetry.arn}/telemetry/*/owner_id=${local.cognito_identity_id}/*"]
  }
}

resource "aws_iam_role_policy" "player" {
  name   = "player-access"
  role   = aws_iam_role.player.id
  policy = data.aws_iam_policy_document.player.json
}

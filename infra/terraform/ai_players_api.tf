# AI player settings must be immutable after creation, which DynamoDB IAM
# conditions cannot express (PutItem overwrites, UpdateItem upserts). Players
# therefore have read-only table access and create or rename AI players through
# this IAM-authenticated API, whose Lambda enforces the rules.

data "archive_file" "ai_players" {
  type        = "zip"
  source_dir  = "${path.module}/functions/ai_players"
  output_path = "${path.module}/.build/ai_players.zip"
  excludes    = ["__pycache__"]
}

resource "aws_cloudwatch_log_group" "ai_players" {
  name              = "/aws/lambda/${local.name_prefix}-ai-players"
  retention_in_days = 30
}

data "aws_iam_policy_document" "lambda_assume_role" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["lambda.amazonaws.com"]
    }
  }
}

resource "aws_iam_role" "ai_players" {
  name               = "${local.name_prefix}-ai-players-api"
  assume_role_policy = data.aws_iam_policy_document.lambda_assume_role.json
}

data "aws_iam_policy_document" "ai_players" {
  statement {
    actions   = ["dynamodb:PutItem", "dynamodb:UpdateItem"]
    resources = [aws_dynamodb_table.ai_player.arn]
  }

  statement {
    actions   = ["logs:CreateLogStream", "logs:PutLogEvents"]
    resources = ["${aws_cloudwatch_log_group.ai_players.arn}:*"]
  }
}

resource "aws_iam_role_policy" "ai_players" {
  name   = "ai-players-api"
  role   = aws_iam_role.ai_players.id
  policy = data.aws_iam_policy_document.ai_players.json
}

resource "aws_lambda_function" "ai_players" {
  function_name    = "${local.name_prefix}-ai-players"
  role             = aws_iam_role.ai_players.arn
  runtime          = "python3.13"
  handler          = "handler.handler"
  filename         = data.archive_file.ai_players.output_path
  source_code_hash = data.archive_file.ai_players.output_base64sha256
  timeout          = 10
  memory_size      = 256

  environment {
    variables = {
      AI_PLAYER_TABLE = aws_dynamodb_table.ai_player.name
    }
  }

  depends_on = [
    aws_cloudwatch_log_group.ai_players,
    aws_iam_role_policy.ai_players,
  ]
}

resource "aws_api_gateway_rest_api" "game" {
  name        = "${local.name_prefix}-game-api"
  description = "Player-facing API for operations that cannot be expressed as direct IAM grants."

  endpoint_configuration {
    types = ["REGIONAL"]
  }
}

resource "aws_api_gateway_resource" "ai_players" {
  rest_api_id = aws_api_gateway_rest_api.game.id
  parent_id   = aws_api_gateway_rest_api.game.root_resource_id
  path_part   = "ai-players"
}

resource "aws_api_gateway_resource" "ai_player" {
  rest_api_id = aws_api_gateway_rest_api.game.id
  parent_id   = aws_api_gateway_resource.ai_players.id
  path_part   = "{ai_player_id}"
}

locals {
  ai_player_routes = {
    create = { resource_id = aws_api_gateway_resource.ai_players.id, method = "POST" }
    rename = { resource_id = aws_api_gateway_resource.ai_player.id, method = "PATCH" }
  }
}

resource "aws_api_gateway_method" "ai_players" {
  for_each = local.ai_player_routes

  rest_api_id   = aws_api_gateway_rest_api.game.id
  resource_id   = each.value.resource_id
  http_method   = each.value.method
  authorization = "AWS_IAM"
}

resource "aws_api_gateway_integration" "ai_players" {
  for_each = local.ai_player_routes

  rest_api_id             = aws_api_gateway_rest_api.game.id
  resource_id             = each.value.resource_id
  http_method             = aws_api_gateway_method.ai_players[each.key].http_method
  integration_http_method = "POST"
  type                    = "AWS_PROXY"
  uri                     = aws_lambda_function.ai_players.invoke_arn
}

resource "aws_lambda_permission" "api_ai_players" {
  statement_id  = "AllowApiGatewayInvoke"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.ai_players.function_name
  principal     = "apigateway.amazonaws.com"
  source_arn    = "${aws_api_gateway_rest_api.game.execution_arn}/*/*/ai-players*"
}

resource "aws_api_gateway_deployment" "game" {
  rest_api_id = aws_api_gateway_rest_api.game.id

  triggers = {
    redeployment = sha1(jsonencode([
      aws_api_gateway_resource.ai_players.id,
      aws_api_gateway_resource.ai_player.id,
      [for route in aws_api_gateway_method.ai_players : route.id],
      [for route in aws_api_gateway_integration.ai_players : route.id],
    ]))
  }

  lifecycle {
    create_before_destroy = true
  }

  depends_on = [aws_api_gateway_integration.ai_players]
}

resource "aws_api_gateway_stage" "game" {
  rest_api_id   = aws_api_gateway_rest_api.game.id
  deployment_id = aws_api_gateway_deployment.game.id
  stage_name    = "v1"
}

resource "aws_api_gateway_method_settings" "game" {
  rest_api_id = aws_api_gateway_rest_api.game.id
  stage_name  = aws_api_gateway_stage.game.stage_name
  method_path = "*/*"

  settings {
    throttling_rate_limit  = 20
    throttling_burst_limit = 40
  }
}

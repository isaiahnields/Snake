output "aws_region" {
  description = "Region the backend is deployed in."
  value       = var.aws_region
}

output "cognito_user_pool_id" {
  description = "Cognito user pool for player accounts."
  value       = aws_cognito_user_pool.players.id
}

output "cognito_user_pool_client_id" {
  description = "Public app client ID used by game builds."
  value       = aws_cognito_user_pool_client.game.id
}

output "cognito_identity_pool_id" {
  description = "Identity pool that exchanges user pool tokens for AWS credentials."
  value       = aws_cognito_identity_pool.players.id
}

output "dynamodb_tables" {
  description = "DynamoDB table names keyed by purpose."
  value = {
    user_play_settings = aws_dynamodb_table.user_play_settings.name
    ai_players         = aws_dynamodb_table.ai_players.name
    user_games         = aws_dynamodb_table.user_games.name
    ai_games           = aws_dynamodb_table.ai_games.name
  }
}

output "ai_players_api_url" {
  description = "SigV4-signed (service execute-api) endpoint: POST to create an AI player, PATCH /{ai_player_id} to rename one."
  value       = "${aws_api_gateway_stage.game.invoke_url}/ai-players"
}

output "telemetry_stream_name" {
  description = "Kinesis data stream that game clients publish telemetry to."
  value       = aws_kinesis_stream.telemetry.name
}

output "telemetry_bucket" {
  description = "S3 bucket receiving telemetry from Firehose."
  value       = aws_s3_bucket.telemetry.id
}

output "gamelift_build_bucket" {
  description = "S3 bucket for uploading zipped multiplayer server builds."
  value       = aws_s3_bucket.gamelift_builds.id
}

output "gamelift_fleet_id" {
  description = "Multiplayer fleet ID, or null until a server build is configured."
  value       = one(aws_gamelift_fleet.server[*].id)
}

output "gamelift_game_session_queue_arn" {
  description = "Game session queue used for match placement."
  value       = aws_gamelift_game_session_queue.multiplayer.arn
}

output "matchmaking_configuration_name" {
  description = "FlexMatch configuration name passed to StartMatchmaking."
  value       = awscc_gamelift_matchmaking_configuration.free_for_all.name
}

output "matchmaking_events_topic_arn" {
  description = "SNS topic receiving FlexMatch ticket events."
  value       = aws_sns_topic.matchmaking_events.arn
}

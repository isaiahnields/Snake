variable "aws_region" {
  description = "AWS region to deploy into. Must support Amazon GameLift Servers and FlexMatch."
  type        = string
  default     = "us-west-2"
}

variable "project" {
  description = "Project name used as a prefix for resource names."
  type        = string
  default     = "snake"
}

variable "environment" {
  description = "Deployment environment name (for example dev or prod)."
  type        = string
  default     = "prod"

  validation {
    condition     = can(regex("^[a-z0-9-]{1,16}$", var.environment))
    error_message = "environment must be 1-16 lowercase letters, digits, or hyphens."
  }
}

variable "deletion_protection" {
  description = "Protect stateful resources (Cognito user pool, DynamoDB tables, S3 buckets) from deletion."
  type        = bool
  default     = true
}

variable "telemetry_stream_mode" {
  description = "Kinesis Data Streams capacity mode: PROVISIONED or ON_DEMAND."
  type        = string
  default     = "PROVISIONED"

  validation {
    condition     = contains(["PROVISIONED", "ON_DEMAND"], var.telemetry_stream_mode)
    error_message = "telemetry_stream_mode must be PROVISIONED or ON_DEMAND."
  }
}

variable "telemetry_shard_count" {
  description = "Shard count when telemetry_stream_mode is PROVISIONED."
  type        = number
  default     = 1
}

variable "telemetry_buffer_interval_seconds" {
  description = "How long Firehose buffers telemetry before writing to S3 (60-900)."
  type        = number
  default     = 300

  validation {
    condition     = var.telemetry_buffer_interval_seconds >= 60 && var.telemetry_buffer_interval_seconds <= 900
    error_message = "telemetry_buffer_interval_seconds must be between 60 and 900."
  }
}

variable "telemetry_infrequent_access_days" {
  description = "Days before telemetry objects transition to S3 Standard-IA (minimum 30)."
  type        = number
  default     = 30

  validation {
    condition     = var.telemetry_infrequent_access_days >= 30
    error_message = "telemetry_infrequent_access_days must be at least 30."
  }
}

variable "telemetry_expiration_days" {
  description = "Days before telemetry objects expire. Set to 0 to keep telemetry forever."
  type        = number
  default     = 0

  validation {
    condition     = var.telemetry_expiration_days == 0 || var.telemetry_expiration_days > var.telemetry_infrequent_access_days
    error_message = "telemetry_expiration_days must be 0 or later than telemetry_infrequent_access_days."
  }
}

variable "gamelift_server_build" {
  description = <<-EOT
    Multiplayer game server build to deploy to a GameLift Servers fleet. Leave null until the
    multiplayer server exists; matchmaking, the queue and the build bucket are still created.
    s3_key is the key of the zipped build inside the gamelift_build_bucket output.
  EOT
  type = object({
    s3_key           = string
    version          = string
    operating_system = optional(string, "AMAZON_LINUX_2023")
    launch_path      = optional(string, "/local/game/snake_server")
    launch_params    = optional(string)
    instance_type    = optional(string, "c5.large")
    fleet_type       = optional(string, "ON_DEMAND")
    processes        = optional(number, 4)
  })
  default = null
}

variable "gamelift_server_port_range" {
  description = "Inclusive port range game server processes listen on."
  type = object({
    from = number
    to   = number
  })
  default = {
    from = 7777
    to   = 7790
  }
}

variable "matchmaking_request_timeout_seconds" {
  description = "Maximum time a FlexMatch ticket may wait for a match."
  type        = number
  default     = 60
}

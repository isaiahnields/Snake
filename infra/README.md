# Backend infrastructure

Terraform for the Snake cloud backend. It is deployed automatically by the
[Deploy backend infrastructure](../.github/workflows/deploy-backend-infrastructure.yml)
workflow.

| Directory | Purpose | Applied by |
| --- | --- | --- |
| [bootstrap](./bootstrap) | Terraform state bucket and the GitHub Actions OIDC roles | An administrator, once, from a workstation |
| [terraform](./terraform) | The game backend | GitHub Actions |

## Services

| Service | Resources |
| --- | --- |
| Amazon Cognito | User pool (email sign-in), public game app client, identity pool that vends AWS credentials to signed-in players |
| Amazon DynamoDB | `manual-play-settings`, `ai-players`, `user-games`, `ai-games` tables |
| Amazon Kinesis + Amazon S3 | Telemetry data stream, Firehose delivery stream, telemetry bucket |
| Amazon GameLift Servers + FlexMatch | Build bucket, fleet alias, game session queue, matchmaking rule set and configuration, matchmaking event topic; server build and fleet once multiplayer exists |
| Amazon API Gateway + AWS Lambda | AI players API, the only way to create or rename AI players |

## Data model

Every player-owned table is partitioned by `owner_id`, the caller's **Cognito
identity ID** (from the identity pool, not the user pool `sub`). The player
IAM role uses `dynamodb:LeadingKeys` so players can only touch their own items,
and `Scan` is not granted.

| Table | Keys | Player access |
| --- | --- | --- |
| `manual-play-settings` | `owner_id` | Get, Put, Update, Delete: always editable |
| `ai-players` | `owner_id`, `ai_player_id` | Get, Query; writes only through the AI players API |
| `user-games` | `owner_id`, `game_id` | Get, Query, Put |
| `ai-games` | `owner_id`, `game_key` = `<ai_player_id>#<train\|test>#<game_id>` | Get, Query, Put |

AI players store `display_name`, `model_settings` (for example learning rate,
gamma and epsilon schedule) and `game_settings` (the environment they play on).
DynamoDB IAM conditions cannot make attributes write-once (`PutItem`
overwrites and `UpdateItem` creates missing items), so players call the AI
players API (`ai_players_api_url` output) with SigV4-signed (`execute-api`)
requests using their identity pool credentials:

| Request | Body | Effect |
| --- | --- | --- |
| `POST /ai-players` | `display_name`, `model_settings`, `game_settings` | Creates the AI player and returns it with its `ai_player_id` |
| `PATCH /ai-players/{ai_player_id}` | `display_name` | Renames an existing AI player; settings cannot be changed |

```json
{
  "display_name": "Greedy Gary",
  "model_settings": { "learning_rate": 0.001, "gamma": 0.95, "epsilon_decay": 0.995 },
  "game_settings": { "grid_width": 30, "grid_height": 30, "wall_collision": "lethal" }
}
```

The Lambda takes the owner from the signed request, never the body.

Game records hold `score` and `telemetry_s3_uri`, the S3 prefix of the game's
telemetry described below.

## Telemetry

Clients put newline-free JSON records on the Kinesis stream
(`telemetry_stream_name` output) using `game_id` as the partition key. Every
record must include string fields:

- `source`: `user`, `ai_train`, `ai_test` or `multiplayer`
- `owner_id`: the Cognito identity ID
- `game_id`: the same ID stored in the game's DynamoDB record

Firehose partitions records and writes gzipped objects to

```text
s3://<telemetry_bucket>/telemetry/source=<source>/owner_id=<owner_id>/game_id=<game_id>/
```

which is the value to store as `telemetry_s3_uri`. Records missing a field land
under `telemetry-errors/`. Characters other than letters, digits, `_`, `-`
(and `:` in `owner_id`) are replaced with `_`. Players can list and read only
prefixes under their own `owner_id`. Objects appear after the Firehose buffer
interval (`telemetry_buffer_interval_seconds`, default 300 seconds).

Kinesis cannot verify the `owner_id` inside a record, so a client could write
telemetry into another player's prefix. Route telemetry through a trusted
service if that matters for leaderboards.

## Multiplayer (in progress)

FlexMatch is configured with a 2-4 player free-for-all rule set
([rule_set.json](./terraform/matchmaking/rule_set.json)) in `WITH_QUEUE` mode.
Until a server build exists, the queue has no destinations and the fleet alias
is a terminal alias, so no instances are running.

When the dedicated server is ready:

1. Zip the build and upload it to the `gamelift_build_bucket` output, for
   example `builds/1.0.0.zip`.
2. Add an `infra/terraform/terraform.tfvars` file and push it to `main`:

   ```hcl
   gamelift_server_build = {
     s3_key      = "builds/1.0.0.zip"
     version     = "1.0.0"
     launch_path = "/local/game/snake_server"
   }
   ```

Changing `version` and `s3_key` deploys a new fleet before the alias switches
over and the old fleet is removed. Matchmaking requests (`StartMatchmaking`)
should come from a trusted backend rather than game clients; ticket events are
published to the `matchmaking_events_topic_arn` SNS topic.

## First-time setup

Requires Terraform 1.10 or later and AWS administrator credentials.

1. Apply the bootstrap stack:

   ```sh
   cd infra/bootstrap
   terraform init
   terraform apply
   ```

   If the account already has a `token.actions.githubusercontent.com` OIDC
   provider, add `-var create_github_oidc_provider=false`. The bootstrap state
   is stored locally in `infra/bootstrap/terraform.tfstate` (ignored by Git);
   keep it somewhere safe.

2. In the GitHub repository settings, add these **repository variables** from
   the bootstrap outputs: `AWS_REGION`, `TF_STATE_BUCKET`, `AWS_PLAN_ROLE_ARN`,
   `AWS_APPLY_ROLE_ARN`.

3. Create a GitHub **environment** named `production` and add required
   reviewers so applies wait for approval.

4. Push to `main` or run the workflow manually.

## Workflow

| Event | Validate | Plan | Apply |
| --- | --- | --- | --- |
| Pull request changing `infra/**` | Yes | Yes, read-only role, plan in the job summary | No |
| Push to `main` changing `infra/**` | Yes | Yes | Yes, in the `production` environment |
| Manual run on `main` | Yes | Yes | Yes |

Plan and apply are skipped until the repository variables are set. Pull
requests from forks only run validation.

## Running locally

```sh
cd infra/terraform
terraform init -backend-config="bucket=<TF_STATE_BUCKET>" -backend-config="region=<AWS_REGION>"
terraform plan -var aws_region=<AWS_REGION>
```

Stateful resources have deletion protection enabled. To tear the stack down,
apply with `-var deletion_protection=false` first, then destroy.

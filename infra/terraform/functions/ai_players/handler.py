"""Creates and renames AI players.

AI player settings are written exactly once at creation; afterwards only the
display name can change. Invoked through API Gateway with AWS_IAM auth, so the
caller's Cognito identity ID comes from the signed request, never the body.

  POST  /ai-players                   {"display_name", "model_settings", "game_settings"}
  PATCH /ai-players/{ai_player_id}    {"display_name"}
"""

import json
import os
import uuid
from datetime import datetime, timezone
from decimal import Decimal

import boto3
from botocore.exceptions import ClientError

TABLE = boto3.resource("dynamodb").Table(os.environ["AI_PLAYERS_TABLE"])

MAX_NAME_LENGTH = 64
MAX_SETTINGS_BYTES = 8192


def _response(status, body):
    return {
        "statusCode": status,
        "headers": {"Content-Type": "application/json"},
        "body": json.dumps(body, default=str),
    }


def _now():
    return datetime.now(timezone.utc).isoformat()


def _display_name(payload):
    name = payload.get("display_name")
    if not isinstance(name, str) or not 0 < len(name.strip()) <= MAX_NAME_LENGTH:
        raise ValueError(f"'display_name' must be 1-{MAX_NAME_LENGTH} characters")
    return name.strip()


def _settings(payload, key):
    value = payload.get(key)
    if not isinstance(value, dict) or not value:
        raise ValueError(f"'{key}' must be a non-empty object")
    encoded = json.dumps(value)
    if len(encoded.encode("utf-8")) > MAX_SETTINGS_BYTES:
        raise ValueError(f"'{key}' exceeds {MAX_SETTINGS_BYTES} bytes")
    # DynamoDB requires Decimal instead of float.
    return json.loads(encoded, parse_float=Decimal)


def _create(owner_id, payload):
    now = _now()
    item = {
        "owner_id": owner_id,
        "ai_player_id": str(uuid.uuid4()),
        "display_name": _display_name(payload),
        "model_settings": _settings(payload, "model_settings"),
        "game_settings": _settings(payload, "game_settings"),
        "created_at": now,
        "updated_at": now,
    }
    TABLE.put_item(Item=item, ConditionExpression="attribute_not_exists(ai_player_id)")
    return _response(201, item)


def _rename(owner_id, ai_player_id, payload):
    try:
        result = TABLE.update_item(
            Key={"owner_id": owner_id, "ai_player_id": ai_player_id},
            UpdateExpression="SET display_name = :name, updated_at = :now",
            ConditionExpression="attribute_exists(ai_player_id)",
            ExpressionAttributeValues={":name": _display_name(payload), ":now": _now()},
            ReturnValues="ALL_NEW",
        )
    except ClientError as error:
        if error.response["Error"]["Code"] == "ConditionalCheckFailedException":
            return _response(404, {"message": "AI player not found"})
        raise
    return _response(200, result["Attributes"])


def handler(event, _context):
    identity = (event.get("requestContext") or {}).get("identity") or {}
    owner_id = identity.get("cognitoIdentityId")
    if not owner_id:
        return _response(403, {"message": "Cognito identity required"})

    try:
        payload = json.loads(event.get("body") or "{}")
        if not isinstance(payload, dict):
            raise ValueError("request body must be a JSON object")

        method = event.get("httpMethod")
        ai_player_id = (event.get("pathParameters") or {}).get("ai_player_id")
        if method == "POST" and not ai_player_id:
            return _create(owner_id, payload)
        if method == "PATCH" and ai_player_id:
            return _rename(owner_id, ai_player_id, payload)
        return _response(405, {"message": "Method not allowed"})
    except (ValueError, json.JSONDecodeError) as error:
        return _response(400, {"message": str(error)})

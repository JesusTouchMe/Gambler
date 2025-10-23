// Copyright 2025 JesusTouchMe

#ifndef DISCORD_MESSAGE_H
#define DISCORD_MESSAGE_H 1

#include "discord/types.h"

typedef struct MessageContent {
    string_t content;
    OPTIONAL(IntegerOrString) nonce;
    boolean_t tts;
    OPTIONAL(MessageReference) message_reference;
    OPTIONAL(boolean_t) enforce_nonce;
} MessageContent;

void InitDefaultMessageContent(MessageContent* message);

int SendMessageEx(snowflake_t channel_id, const MessageContent* message);

int SendMessage(snowflake_t channel_id, const char* message);
int SendReply(snowflake_t channel_id, snowflake_t message_id, const char* message);

#endif //DISCORD_MESSAGE_H
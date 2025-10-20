// Copyright 2025 JesusTouchMe

#ifndef DISCORD_TYPES_H
#define DISCORD_TYPES_H 1

#include "internal/memory.h"

#include "utils/jsonutils.h"

#include <stdint.h>
#include <time.h>

// define types they mention in the docs

typedef _Bool boolean_t;
typedef struct Color color_t;
typedef double float_t;
typedef uint64_t integer_t;
typedef char null_t; // what the fuck discord. JUST USE BOOL (https://discord.com/developers/docs/topics/permissions#role-object-role-tags-structure)
typedef uint64_t snowflake_t;
typedef const char* string_t;
typedef time_t iso8601_timestamp_t;

struct Color {
    unsigned char r, g, b;
};

typedef struct IntegerOrString {
    boolean_t is_string; // true if string

    union {
        integer_t integer;
        string_t string;
    };
} IntegerOrString;

// primitive array types blaaaah

typedef struct ColorArray {
    const color_t* colors;
    size_t count;
} ColorArray;

typedef struct SnowflakeArray {
    const snowflake_t* snowflakes;
    size_t count;
} SnowflakeArray;

typedef struct StringArray {
    const string_t* strings;
    size_t count;
} StringArray;

// special data types because discord is made in hashmap language

typedef enum OptionalState {
    OPTION_ABSENT,
    OPTION_NULL,
    OPTION_EXISTS,
} OptionalState;

#define PAIR(A, B) struct { A first; B second; }
#define DICTIONARY(K, V) struct { const PAIR(K, V)* entries; size_t size; }
#define OPTIONAL(T) struct { OptionalState state; T value; }

// define the big structures

typedef struct Nameplate {
    snowflake_t sku_id;
    string_t asset;
    string_t label;
    string_t palette;
} Nameplate;

typedef struct Collectibles {
    OPTIONAL(Nameplate) nameplate;
} Collectibles;

typedef struct AvatarDecorationData {
    string_t asset;
    snowflake_t sku_id;
} AvatarDecorationData;

typedef struct UserPrimaryGuild {
    OPTIONAL(snowflake_t) identity_guild_id;
    OPTIONAL(boolean_t) identity_enabled;
    OPTIONAL(string_t) tag;
    OPTIONAL(string_t) badge;
} UserPrimaryGuild;

typedef enum UserFlags {
    STAFF = 1 << 0,
    PARTNER = 1 << 1,
    HYPESQUAD = 1 << 2,
    BUG_HUNTER_LEVEL_1 = 1 << 3,
    HYPESQUAD_ONLINE_HOUSE_1 = 1 << 6,
    HYPESQUAD_ONLINE_HOUSE_2 = 1 << 7,
    HYPESQUAD_ONLINE_HOUSE_3 = 1 << 8,
    PREMIUM_EARLY_SUPPORTER = 1 << 9,
    TEAM_PSEUDO_USER = 1 << 10,
    BUG_HUNTER_LEVEL_2 = 1 << 14,
    VERIFIED_BOT = 1 << 16,
    VERIFIED_DEVELOPER = 1 << 17,
    CERTIFIED_MODERATOR = 1 << 18,
    BOT_HTTP_INTERACTIONS = 1 << 19,
    ACTIVE_DEVELOPER = 1 << 22,
} UserFlags;

typedef struct User {
    snowflake_t id;
    string_t username;
    string_t discriminator;
    OPTIONAL(string_t) global_name;
    OPTIONAL(string_t) avatar;
    OPTIONAL(boolean_t) bot;
    OPTIONAL(boolean_t) system;
    OPTIONAL(boolean_t) mfa_enabled;
    OPTIONAL(string_t) banner;
    OPTIONAL(integer_t) accent_color;
    OPTIONAL(string_t) locale;
    OPTIONAL(boolean_t) verified;
    OPTIONAL(string_t) email;
    OPTIONAL(integer_t) flags;
    OPTIONAL(integer_t) premium_type;
    OPTIONAL(integer_t) public_flags;
    OPTIONAL(AvatarDecorationData) avatar_decoration_data;
    OPTIONAL(Collectibles) collectibles;
    OPTIONAL(UserPrimaryGuild) primary_guild;
} User;

typedef struct UserArray {
    const User* users;
    size_t count;
} UserArray;


typedef enum AttachmentFlags {
    IS_REMIX = 1 << 2,
} AttachmentFlags;

typedef struct Attachment {
    snowflake_t id;
    string_t filename;
    OPTIONAL(string_t) title;
    OPTIONAL(string_t) description;
    OPTIONAL(string_t) content_type;
    integer_t size;
    string_t url;
    string_t proxy_url;
    OPTIONAL(integer_t) height;
    OPTIONAL(integer_t) width;
    OPTIONAL(boolean_t) ephemeral;
    OPTIONAL(float_t) duration_secs;
    OPTIONAL(string_t) waveform;
    OPTIONAL(integer_t) flags;
} Attachment;

typedef struct AttachmentArray {
    const Attachment* attachments;
    size_t count;
} AttachmentArray;

typedef struct EmbedThumbnail {
    string_t url;
    OPTIONAL(string_t) proxy_url;
    OPTIONAL(integer_t) height;
    OPTIONAL(integer_t) width;
} EmbedThumbnail;

typedef struct EmbedVideo {
    OPTIONAL(string_t) url;
    OPTIONAL(string_t) proxy_url;
    OPTIONAL(integer_t) height;
    OPTIONAL(integer_t) width;
} EmbedVideo;

typedef struct EmbedImage {
    string_t url;
    OPTIONAL(string_t) proxy_url;
    OPTIONAL(integer_t) height;
    OPTIONAL(integer_t) width;
} EmbedImage;

typedef struct EmbedProvider {
    OPTIONAL(string_t) name;
    OPTIONAL(string_t) url;
} EmbedProvider;

typedef struct EmbedAuthor {
    string_t name;
    OPTIONAL(string_t) url;
    OPTIONAL(string_t) icon_url;
    OPTIONAL(string_t) proxy_icon_url;
} EmbedAuthor;

typedef struct EmbedFooter {
    string_t text;
    OPTIONAL(string_t) icon_url;
    OPTIONAL(string_t) proxy_icon_url;
} EmbedFooter;

typedef struct EmbedField {
    string_t name;
    string_t value;
    OPTIONAL(boolean_t) _inline; // json field is just inline
} EmbedField;

typedef struct EmbedFieldArray {
    const EmbedField* fields;
    size_t count;
} EmbedFieldArray;

typedef struct Embed {
    OPTIONAL(string_t) title;
    OPTIONAL(string_t) type;
    OPTIONAL(string_t) description;
    OPTIONAL(string_t) url;
    OPTIONAL(iso8601_timestamp_t) timestamp;
    OPTIONAL(integer_t) color;
    OPTIONAL(EmbedFooter) footer;
    OPTIONAL(EmbedImage) image;
    OPTIONAL(EmbedThumbnail) thumbnail;
    OPTIONAL(EmbedVideo) video;
    OPTIONAL(EmbedProvider) provider;
    OPTIONAL(EmbedAuthor) author;
    OPTIONAL(EmbedFieldArray) fields;
} Embed;

typedef struct EmbedArray {
    const Embed* embeds;
    size_t count;
} EmbedArray;

typedef struct Emoji {
    OPTIONAL(snowflake_t) id;
    OPTIONAL(string_t) name;
    OPTIONAL(SnowflakeArray) roles;
    OPTIONAL(User) user;
    OPTIONAL(boolean_t) require_colons;
    OPTIONAL(boolean_t) managed;
    OPTIONAL(boolean_t) animated;
    OPTIONAL(boolean_t) available;
} Emoji;

typedef struct EmojiArray {
    const Emoji* emojis;
    size_t count;
} EmojiArray;

typedef struct RoleTags {
    OPTIONAL(snowflake_t) bot_id;
    OPTIONAL(snowflake_t) integration_id;
    OPTIONAL(null_t) premium_subscriber;
    OPTIONAL(snowflake_t) subscription_listing_id;
    OPTIONAL(null_t) available_for_purchase;
    OPTIONAL(null_t) guild_connections;
} RoleTags;

typedef struct RoleColors {
    integer_t primary_color;
    OPTIONAL(integer_t) secondary_color;
    OPTIONAL(integer_t) tertiary_color;
} RoleColors;

typedef enum RoleFlags {
    IN_PROMPT = 1 << 0,
} RoleFlags;

typedef struct Role {
    snowflake_t id;
    string_t name;
    integer_t color; // deprecated
    RoleColors colors; // use this instead
    boolean_t hoist;
    OPTIONAL(string_t) icon;
    OPTIONAL(string_t) unicode_emoji;
    integer_t position;
    string_t permissions;
    boolean_t managed;
    boolean_t mentionable;
    OPTIONAL(RoleTags) tags;
    integer_t flags;
} Role;

typedef struct RoleArray {
    const Role* roles;
    size_t count;
} RoleArray;

typedef struct RoleSubscriptionData {
    snowflake_t role_subscription_listing_id;
    string_t tier_name;
    integer_t total_months_subscribed;
    boolean_t is_renewal;
} RoleSubscriptionData;

typedef struct WelcomeScreenChannel {
    snowflake_t channel_id;
    string_t description;
    OPTIONAL(snowflake_t) emoji_id;
    OPTIONAL(string_t) emoji_name;
} WelcomeScreenChannel;

typedef struct WelcomeScreenChannelArray {
    const WelcomeScreenChannel* channels; // i know docs say up to 5, but ts is future proof and could use less memory
    size_t count;
} WelcomeScreenChannelArray;

typedef struct WelcomeScreen {
    OPTIONAL(string_t) description;
    WelcomeScreenChannelArray welcome_channels;
} WelcomeScreen;

typedef enum StickerType {
    STANDARD = 1,
    GUILD = 2
} StickerType;

typedef enum StickerFormatType {
    PNG = 1,
    APNG = 2,
    LOTTIE = 3,
    GIF = 4,
} StickerFormatType;

typedef struct Sticker {
    snowflake_t id;
    OPTIONAL(snowflake_t) pack_id;
    string_t name;
    OPTIONAL(string_t) description;
    string_t tags;
    integer_t type;
    integer_t format_type;
    OPTIONAL(boolean_t) available;
    OPTIONAL(User) user;
    OPTIONAL(integer_t) sort_value;
} Sticker;

typedef struct StickerArray {
    const Sticker* stickers;
    size_t count;
} StickerArray;

typedef struct StickerItem {
    snowflake_t id;
    string_t name;
    integer_t format_type;
} StickerItem;

typedef struct StickerItemArray {
    const StickerItem* items;
    size_t count;
} StickerItemArray;

typedef struct IncidentsData {
    OPTIONAL(iso8601_timestamp_t) invites_disabled_until;
    OPTIONAL(iso8601_timestamp_t) dms_disabled_until;
    OPTIONAL(iso8601_timestamp_t) dm_spam_detected_at;
    OPTIONAL(iso8601_timestamp_t) raid_detected_at;
} IncidentsData;

typedef enum DefaultMessageNotificationLevel {
    ALL_MESSAGES = 0,
    ONLY_MENTIONS = 1,
} DefaultMessageNotificationLevel;

typedef enum ExplicitContentFilterLevel {
    EXPLICIT_FILTER_DISABLED = 0, // the name in docs is DISABLES, but i can't do that
    MEMBERS_WITHOUT_ROLES = 1,
    ALL_MEMBERS = 2,
} ExplicitContentFilterLevel;

// names in docs are without the MFA_ prefix
typedef enum MFALevel {
    MFA_NONE = 0,
    MFA_ELEVATED = 1,
} MFALevel;

// names in docs are without the VERIFICATION_ prefix
typedef enum VerificationLevel {
    VERIFICATION_NONE = 0,
    VERIFICATION_LOW = 1,
    VERIFICATION_MEDIUM = 2,
    VERIFICATION_HIGH = 3,
    VERIFICATION_VERY_HIGH = 4,
} VerificationLevel;

// names in docs are without the NSFW_ prefix
typedef enum GuildNSFWLevel {
    NSFW_DEFAULT = 0,
    NSFW_EXPLICIT = 1,
    NSFW_SAFE = 2,
    NSFW_AGE_RESTRICTED = 3,
} GuildNSFWLevel;


// names in docs are without the PREMIUM_ prefix
typedef enum PremiumTier {
    PREMIUM_NONE = 0,
    PREMIUM_TIER_1 = 1,
    PREMIUM_TIER_2 = 2,
    PREMIUM_TIER_3 = 3,
} PremiumTier;

typedef enum SystemChannelFlags {
    SUPPRESS_JOIN_NOTIFICATIONS = 1 << 0,
    SUPPRESS_PREMIUM_NOTIFICATIONS = 1 << 1,
    SUPPRESS_GUILD_REMINDER_NOTIFICATIONS = 1 << 2,
    SUPPRESS_JOIN_NOTIFICATION_REPLIES = 1 << 3,
    SUPPRESS_ROLE_SUBSCRIPTION_PURCHASE_NOTIFICATIONS = 1 << 4,
    SUPPRESS_ROLE_SUBSCRIPTION_PURCHASE_NOTIFICATION_REPLIES = 1 << 5,
} SystemChannelFlags;

typedef struct Guild {
    snowflake_t id;
    string_t name;
    OPTIONAL(string_t) icon;
    OPTIONAL(string_t) icon_hash;
    OPTIONAL(string_t) splash;
    OPTIONAL(string_t) discovery_splash;
    OPTIONAL(boolean_t) owner;
    snowflake_t owner_id;
    OPTIONAL(string_t) permissions;
    OPTIONAL(string_t) region;
    OPTIONAL(snowflake_t) afk_channel_id;
    integer_t afk_timeout;
    OPTIONAL(boolean_t) widget_enabled;
    OPTIONAL(snowflake_t) widget_channel_id;
    integer_t verification_level;
    integer_t default_message_notifications;
    integer_t explicit_content_filter;
    RoleArray roles;
    EmojiArray emojis;
    StringArray features;
    integer_t mfa_level;
    OPTIONAL(snowflake_t) application_id;
    OPTIONAL(snowflake_t) system_channel_id;
    integer_t system_channel_flags;
    OPTIONAL(snowflake_t) rules_channel_id;
    OPTIONAL(integer_t) max_presences;
    OPTIONAL(integer_t) max_members;
    OPTIONAL(string_t) vanity_url_code;
    OPTIONAL(string_t) description;
    OPTIONAL(string_t) banner;
    integer_t premium_tier;
    OPTIONAL(integer_t) premium_subscription_count;
    string_t preferred_locale;
    OPTIONAL(snowflake_t) public_updates_channel_id;
    OPTIONAL(integer_t) max_video_channel_users;
    OPTIONAL(integer_t) max_stage_video_channel_users;
    OPTIONAL(integer_t) approximate_member_count;
    OPTIONAL(WelcomeScreen) welcome_screen;
    integer_t nsfw_level;
    OPTIONAL(StickerArray) stickers;
    boolean_t premium_progress_bar_enabled;
    OPTIONAL(snowflake_t) safety_alerts_channel_id;
    OPTIONAL(IncidentsData) incidents_data;
} Guild;

typedef struct GuildMember {
    OPTIONAL(User) user;
    OPTIONAL(string_t) nick;
    OPTIONAL(string_t) avatar;
    OPTIONAL(string_t) banner;
    SnowflakeArray roles;
    OPTIONAL(iso8601_timestamp_t) joined_at;
    OPTIONAL(iso8601_timestamp_t) premium_since;
    boolean_t deaf;
    boolean_t mute;
    integer_t flags;
    OPTIONAL(boolean_t) pending;
    OPTIONAL(string_t) permissions;
    OPTIONAL(iso8601_timestamp_t) communication_disabled_until;
    OPTIONAL(AvatarDecorationData) avatar_decoration_data;
} GuildMember;

typedef enum MembershipState {
    INVITED = 1,
    ACCEPTED = 2,
} MembershipState;

typedef struct TeamMember {
    integer_t membership_state;
    snowflake_t team_id;
    User user;
    string_t role;
} TeamMember;

typedef struct TeamMemberArray {
    const TeamMember* members;
    size_t count;
} TeamMemberArray;

typedef struct Team {
    OPTIONAL(string_t) icon;
    snowflake_t id;
    TeamMemberArray members;
    string_t name;
    snowflake_t owner_user_id;
} Team;

typedef struct InstallParams {
    StringArray scopes;
    string_t permissions;
} InstallParams;

typedef enum ApplicationIntegrationType {
    GUILD_INSTALL = 0,
    USER_INSTALL = 1,
} ApplicationIntegrationType;

typedef struct ApplicationIntegrationTypeConfiguration {
    OPTIONAL(InstallParams) oauth2_install_params;
} ApplicationIntegrationTypeConfiguration;

typedef struct Application {
    snowflake_t id;
    string_t name;
    OPTIONAL(string_t) icon;
    string_t description;
    OPTIONAL(StringArray) rpc_origins;
    boolean_t bot_public;
    boolean_t bot_require_code_grant;
    OPTIONAL(User) bot;
    OPTIONAL(string_t) terms_of_service_url;
    OPTIONAL(string_t) privacy_policy_url;
    OPTIONAL(User) owner;
    string_t verify_key;
    OPTIONAL(Team) team;
    OPTIONAL(snowflake_t) guild_id;
    OPTIONAL(Guild) guild;
    OPTIONAL(snowflake_t) primary_sku_id;
    OPTIONAL(string_t) slug;
    OPTIONAL(string_t) cover_image;
    OPTIONAL(integer_t) flags;
    OPTIONAL(integer_t) approximate_guild_count;
    OPTIONAL(integer_t) approximate_user_install_count;
    OPTIONAL(integer_t) approximate_user_authorization_count;
    OPTIONAL(StringArray) redirect_uris;
    OPTIONAL(string_t) interactions_endpoint_url;
    OPTIONAL(string_t) role_connections_verification_url;
    OPTIONAL(string_t) event_webhooks_url;
    integer_t event_webhook_status;
    OPTIONAL(StringArray) event_webhooks_types;
    OPTIONAL(StringArray) tags;
    OPTIONAL(InstallParams) install_params;
    OPTIONAL(DICTIONARY(integer_t, ApplicationIntegrationTypeConfiguration)) integration_types_config;
    OPTIONAL(string_t) custom_install_url;
} Application;

typedef struct ReactionCountDetails {
    integer_t burst;
    integer_t normal;
} ReactionCountDetails;

typedef struct Reaction {
    integer_t count;
    ReactionCountDetails count_details;
    boolean_t me;
    boolean_t me_burst;
    Emoji emoji;
    ColorArray burst_colors;
} Reaction;

typedef struct ReactionArray {
    const Reaction* reactions;
    size_t count;
} ReactionArray;

typedef struct DefaultReaction {
    OPTIONAL(snowflake_t) emoji_id;
    OPTIONAL(string_t) emoji_name;
} DefaultReaction;

typedef struct ThreadMetadata {
    boolean_t archived;
    iso8601_timestamp_t archive_timestamp;
    boolean_t locked;
    OPTIONAL(boolean_t) invitable;
    OPTIONAL(iso8601_timestamp_t) create_timestamp;
} ThreadMetadata;

typedef struct ThreadMember {
    OPTIONAL(snowflake_t) id;
    OPTIONAL(snowflake_t) user_id;
    iso8601_timestamp_t join_timestamp;
    integer_t flags;
    OPTIONAL(GuildMember) member;
} ThreadMember;

typedef struct ForumTag {
    snowflake_t id;
    string_t name;
    boolean_t moderated;
    OPTIONAL(snowflake_t) emoji_id;
    OPTIONAL(string_t) emoji_name;
} ForumTag;

typedef struct ForumTagArray {
    const ForumTag* tags;
    size_t count;
} ForumTagArray;

typedef enum ChannelType {
    GUILD_TEXT = 0,
    DM = 1,
    GUILD_VOICE = 2,
    GROUP_DM = 3,
    GUILD_CATEGORY = 4,
    GUILD_ANNOUNCEMENT = 5,
    ANNOUNCEMENT_THREAD = 10,
    PUBLIC_THREAD = 11,
    PRIVATE_THReAD = 12,
    GUILD_STAGE_VOICE = 13,
    GUILD_DIRECTORY = 14,
    GUILD_FORUM = 15,
    GUILD_MEDIA = 16,
} ChannelType;

// no QUALITY_ prefix in api doc
typedef enum VideoQualityMode {
    QUALITY_AUTO = 1,
    QUALITY_FULL = 2,
} VideoQualityMode;

typedef enum ChannelFlag {
    PINNED = 1 << 1,
    REQUIRE_TAG = 1 << 4,
    HIDE_MEDIA_DOWNLOAD_OPTIONS = 1 << 15,
} ChannelFlag;

// no SORT_ prefix in api doc
typedef enum SortOrderType {
    SORT_LATEST_ACTIVITY = 0,
    SORT_CREATION_DATE = 1,
} SortOrderType;

// FORUM_LAYOUT_ prefix not in api doc
typedef enum ForumLayoutType {
    FORUM_LAYOUT_NOT_SET = 0,
    FORUM_LAYOUT_LIST_VIEW = 1,
    FORUM_LAYOUT_GALLERY_VIEW = 2,
} ForumLayoutType;

typedef struct ChannelMention {
    snowflake_t id;
    snowflake_t guild_id;
    integer_t type;
    string_t name;
} ChannelMention;

typedef struct ChannelMentionArray {
    const ChannelMention* channel_mentions;
    size_t count;
} ChannelMentionArray;

typedef struct PermissionOverwrite {
    snowflake_t id;
    integer_t type;
    string_t allow;
    string_t deny;
} PermissionOverwrite;

typedef struct PermissionOverwriteArray {
    const PermissionOverwrite* overwrites;
    size_t count;
} PermissionOverwriteArray;

typedef struct Channel {
    snowflake_t id;
    integer_t type;
    OPTIONAL(snowflake_t) guild_id;
    OPTIONAL(integer_t) position;
    OPTIONAL(PermissionOverwriteArray) permission_overwrites;
    OPTIONAL(string_t) name;
    OPTIONAL(string_t) topic;
    OPTIONAL(boolean_t) nsfw;
    OPTIONAL(snowflake_t) last_message_id;
    OPTIONAL(integer_t) bitrate;
    OPTIONAL(integer_t) user_limit;
    OPTIONAL(integer_t) rate_limit_per_user;
    OPTIONAL(UserArray) recipients;
    OPTIONAL(string_t) icon;
    OPTIONAL(snowflake_t) owner_id;
    OPTIONAL(snowflake_t) application_id;
    OPTIONAL(boolean_t) managed;
    OPTIONAL(snowflake_t) parent_id;
    OPTIONAL(iso8601_timestamp_t) last_pin_timestamp;
    OPTIONAL(string_t) rtc_region;
    OPTIONAL(integer_t) video_quality_mode;
    OPTIONAL(integer_t) message_count;
    OPTIONAL(integer_t) member_count;
    OPTIONAL(ThreadMember) thread_metadata;
    OPTIONAL(ThreadMember) member;
    OPTIONAL(integer_t) default_auto_archive_duration;
    OPTIONAL(string_t) permissions;
    OPTIONAL(integer_t) flags;
    OPTIONAL(integer_t) total_message_sent;
    OPTIONAL(ForumTagArray) available_tags;
    OPTIONAL(SnowflakeArray) applied_tags;
    OPTIONAL(DefaultReaction) default_reaction_emoji;
    OPTIONAL(integer_t) default_thread_rate_limit_per_user;
    OPTIONAL(integer_t) default_sort_order;
    OPTIONAL(integer_t) default_forum_layout;
} Channel;

typedef struct PollMedia {
    OPTIONAL(string_t) text;
    OPTIONAL(Emoji) emoji;
} PollMedia;

typedef struct PollAnswer {
    integer_t answer_id;
    PollMedia poll_media;
} PollAnswer;

typedef struct PollAnswerArray {
    const PollAnswer* answers;
    size_t count;
} PollAnswerArray;

typedef struct PollAnswerCount {
    integer_t id;
    integer_t count;
    boolean_t me_voted;
} PollAnswerCount;

typedef struct PollAnswerCountArray {
    const PollAnswerCount* answers;
    size_t count;
} PollAnswerCountArray;

typedef struct PollResults {
    boolean_t is_finalized;
    PollAnswerCountArray answer_counts;
} PollResults;

// POLL_ prefix not in api doc
typedef enum PollLayoutType {
    POLL_DEFAULT = 1,
} PollLayoutType;

typedef struct Poll {
    PollMedia question;
    PollAnswerArray answers;
    OPTIONAL(iso8601_timestamp_t) expiry;
    boolean_t allow_multiselect;
    integer_t layout_type;
    OPTIONAL(PollResults) results;
} Poll;

typedef struct ResolvedData {
    OPTIONAL(DICTIONARY(snowflake_t, User)) users;
    OPTIONAL(DICTIONARY(snowflake_t, GuildMember)) members;
    OPTIONAL(DICTIONARY(snowflake_t, Role)) roles;
    OPTIONAL(DICTIONARY(snowflake_t, Channel)) channels;
    OPTIONAL(DICTIONARY(snowflake_t, const struct Member*)) messages;
    OPTIONAL(DICTIONARY(snowflake_t, Attachment)) attachments;
} ResolvedData;

typedef enum MessageActivityType {
    JOIN = 1,
    SPECTATE = 2,
    LISTEN = 3,
    JOIN_REQUEST = 5,
} MessageActivityType;

typedef struct MessageActivity {
    integer_t type;
    OPTIONAL(string_t) party_id;
} MessageActivity;

typedef enum MessageType {
    MESSAGE_DEFAULT = 0, // not the actual name, but i'd anger folks by making it DEFAULT
    RECIPIENT_ADD = 1,
    RECIPIENT_REMOVE = 2,
    CALL = 3,
    CHANNEL_NAME_CHANGE = 4,
    CHANNEL_ICON_CHANGE = 5,
    CHANNEL_PINNED_MESSAGE = 6,
    USER_JOIN = 7,
    GUILD_BOOST = 8,
    GUILD_BOOST_TIER_1 = 9,
    GUILD_BOOST_TIER_2 = 10,
    GUILD_BOOST_TIER_3 = 11,
    CHANNEL_FOLLOW_ADD = 12,
    GUILD_DISCOVERY_DISQUALIFIED = 14,
    GUILD_DISCOVERY_REQUALIFIED = 15,
    GUILD_DISCOVERY_GRACE_PERIOD_INITIAL_WARNING = 16,
    GUILD_DISCOVERY_GRACE_PERIOD_FINAL_WARNING = 17,
    THREAD_CREATED = 18,
    REPLY = 19,
    CHAT_INPUT_COMMAND = 20,
    THREAD_STARTER_MESSAGE = 21,
    GUILD_INVITE_REMINDER = 22,
    CONTEXT_MENU_COMMAND = 23,
    AUTO_MODERATION_ACTION = 24,
    ROLE_SUBSCRIPTION_PURCHASE = 25,
    INTERACTION_PREMIUM_UPSELL = 26,
    STAGE_START = 27,
    STAGE_END = 28,
    STAGE_SPEAKER = 29,
    STAGE_TOPIC = 31,
    GUILD_APPLICATION_PREMIUM_SUBSCRIPTION = 32,
    GUILD_INCIDENT_ALERT_MODE_ENABLED = 36,
    GUILD_INCIDENT_ALERT_MODE_DISABLED = 37,
    GUILD_INCIDENT_REPORT_RAID = 38,
    GUILD_INCIDENT_REPORT_FALSE_ALARM = 39,
    PURCHASE_NOTIFICATION = 44,
    POLL_RESULT = 46,
} MessageType;

// MESSAGE_REFERENCE_ prefix not in api doc
typedef enum MessageReferenceType {
    MESSAGE_REFERENCE_DEFAULT = 0,
    MESSAGE_REFERENCE_FORWARD = 1,
} MessageReferenceType;

typedef enum InteractionType {
    INTERACTION_PING = 1, // INTERACTION_ prefix not in api doc
    APPLICATION_COMMAND = 2,
    MESSAGE_COMPONENT = 3,
    APPLICATION_COMMAND_AUTOCOMPLETE = 4,
    MODAL_SUBMIT = 5,
} InteractionType;

typedef struct MessageCall {
    SnowflakeArray participants;
    OPTIONAL(iso8601_timestamp_t) ended_timestamp;
} MessageCall;

typedef struct MessageReference {
    OPTIONAL(integer_t) type;
    OPTIONAL(snowflake_t) message_id;
    OPTIONAL(snowflake_t) channel_id;
    OPTIONAL(snowflake_t) guild_id;
    OPTIONAL(boolean_t) fail_if_not_exists;
} MessageReference;

typedef struct MessageSnapshot {
    const struct Message* message; // what horrid design choices, discord
} MessageSnapshot;

typedef struct MessageSnapshotArray {
    const MessageSnapshot snapshots;
    size_t count;
} MessageSnapshotArray;

typedef struct MessageInteractionMetadata {
    snowflake_t id;
    integer_t interaction_type;
    User user;
    DICTIONARY(integer_t, string_t) authorizing_integration_owners; // i REALLY don't wanna implement a value type, so figure it out yourself
    OPTIONAL(snowflake_t) original_message_id;
    OPTIONAL(User) target_user;
    OPTIONAL(snowflake_t) target_message_id;
    OPTIONAL(snowflake_t) original_response_message_id;
    OPTIONAL(snowflake_t) interacted_message_id;
    OPTIONAL(const struct MessageInteractionMetadata*) triggering_interaction_metadata;
} MessageInteractionMetadata;

typedef struct MessageInteraction {
    snowflake_t id;
    integer_t type;
    string_t name;
    User user;
    OPTIONAL(GuildMember) member;
} MessageInteraction;

typedef struct Message {
    snowflake_t id;
    snowflake_t channel_id;
    User author;
    string_t content;
    iso8601_timestamp_t timestamp;
    OPTIONAL(time_t) edited_timestamp;
    boolean_t tts;
    boolean_t mention_everyone;
    UserArray mentions;
    SnowflakeArray mention_roles;
    OPTIONAL(ChannelMentionArray) mention_channels;
    AttachmentArray attachments;
    EmbedArray embeds;
    OPTIONAL(ReactionArray) reactions;
    OPTIONAL(IntegerOrString) nonce;
    boolean_t pinned;
    OPTIONAL(snowflake_t) webhook_id;
    integer_t type;
    OPTIONAL(MessageActivity) activity;
    OPTIONAL(Application) application;
    OPTIONAL(snowflake_t) application_id;
    OPTIONAL(integer_t) flags;
    OPTIONAL(MessageReference) message_reference;
    OPTIONAL(MessageSnapshotArray) message_snapshots;
    OPTIONAL(const struct Message*) referenced_message; // I HATE YOU DISCORD
    OPTIONAL(MessageInteractionMetadata) interaction_metadata;
    OPTIONAL(MessageInteraction) interaction;
    OPTIONAL(Channel) thread;
    // TODO: components
    OPTIONAL(StickerItemArray) sticker_items;
    OPTIONAL(StickerArray) stickers;
    OPTIONAL(integer_t) position;
    OPTIONAL(RoleSubscriptionData) role_subscription_data;
    OPTIONAL(ResolvedData) resolved;
    OPTIONAL(Poll) poll;
    OPTIONAL(MessageCall) call;
} Message;

const Message* ParseMessage(Arena* arena, const char* json, jsmntok_t* tokens, JsonObject message);

#endif // DISCORD_TYPES_H

// data.hpp - roster, items, and dialogue for Îlot14 Island.
// Everything here is plain data: edit freely (looks, ages, personalities, lines).
#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum Pers { P_CHEERFUL, P_SHY, P_TSUNDERE, P_CALM, P_GOOFY, P_BOSSY, P_CURIOUS, P_SASSY, P_CHILL, P_TINY, P_COUNT };
static const char* PERS_NAME[P_COUNT] = {"Cheerful", "Shy", "Tsundere", "Calm", "Goofy", "Bossy", "Curious", "Sassy", "Chill", "Tiny"};

struct CDef {
    const char* name; const char* full; const char* code; int age;
    int skin, hair, hairStyle; uint32_t top, bottom; int pers;
};
// NOTE: looks/personalities are placeholders until you supply real character art. Ages follow the canon notes.
static const uint32_t SKINS[5] = {0xF5D0B0, 0xE8B98F, 0xD39B6E, 0xA9744F, 0x7B5233};
static const uint32_t HAIRS[6] = {0x15110D, 0x3A2615, 0x6B4423, 0xB8742E, 0x2B2B2B, 0xD9B36A};
static const CDef ROSTER[] = {
    {"Amir", "Amir", "L-001", 13, 1, 0, 1, 0xF97316, 0x2A2A33, P_TSUNDERE},
    {"Raceem", "Raceem", "L-002", 12, 2, 1, 0, 0x3B82F6, 0x1F2937, P_CHEERFUL},
    {"Adam", "Adam", "L-003", 12, 1, 0, 0, 0x10B981, 0x374151, P_CALM},
    {"Aya", "Aya", "L-005", 10, 3, 0, 2, 0xEC4899, 0x1E3A8A, P_SHY},
    {"Basmala", "Basmala", "L-006", 12, 1, 2, 2, 0x8B5CF6, 0x111827, P_BOSSY},
    {"Fares", "Fares", "OL-007", 11, 4, 0, 5, 0xEAB308, 0x1F2937, P_GOOFY},
    {"Adam C.", "Adam Chermatti", "L-007", 11, 2, 1, 4, 0x06B6D4, 0x44403C, P_CURIOUS},
    {"Arwa", "Arwa", "M-001", 12, 1, 1, 3, 0xEF4444, 0x1F2937, P_CALM},
    {"Dalia", "Dalia Bachouche", "M-002", 12, 2, 0, 2, 0x14B8A6, 0x292524, P_SASSY},
    {"Dania", "Dania", "M-003", 11, 3, 2, 3, 0xF472B6, 0x1E293B, P_CHEERFUL},
    {"Gabriel", "Gabriel", "M-004", 12, 1, 1, 1, 0x84CC16, 0x1F2937, P_GOOFY},
    {"Youcef", "Youcef", "M-005", 12, 4, 0, 4, 0x64748B, 0x0F172A, P_CHILL},
    {"Amine", "Amine", "S-001", 13, 2, 0, 0, 0xF59E0B, 0x334155, P_CHILL},
    {"Haydar", "Haydar", "S-002", 11, 3, 0, 0, 0x0EA5E9, 0x1F2937, P_SHY},
    {"Anais", "Anais", "R-001", 12, 1, 3, 2, 0xA855F7, 0x1F2937, P_CURIOUS},
    {"Maysanne", "Maysanne", "R-002", 9, 2, 0, 3, 0xFB7185, 0x3730A3, P_SHY},
    {"Jilali", "Jilali", "R-003", 20, 4, 0, 4, 0x78716C, 0x1C1917, P_CALM},
    {"Alicia", "Alicia", "UC-001", 5, 1, 0, 3, 0xFDA4AF, 0x7C3AED, P_TINY},
    {"Abdullah", "Abdullah", "UC-002", 6, 2, 0, 0, 0x38BDF8, 0x1E40AF, P_TINY},
    {"Jalil", "Jalil", "UC-003", 4, 3, 0, 5, 0xA3E635, 0x365314, P_TINY},
    {"Adam A.", "Adam Abdellani", "UC-004", 8, 1, 1, 0, 0xFB923C, 0x44403C, P_CURIOUS},
    {"Amira", "Amira", "UC-005", 8, 2, 0, 2, 0xC084FC, 0x581C87, P_CHEERFUL},
    {"Anes", "Anes", "C-001", 10, 3, 0, 1, 0x22C55E, 0x14532D, P_GOOFY},
    {"Iyad", "Iyad", "C-005", 10, 1, 0, 0, 0x2563EB, 0x1E3A8A, P_CHILL},
    {"Amyas", "Amyas Bachouche", "T-001", 19, 2, 0, 0, 0xF9A8D4, 0x1F2937, P_CHILL},
    {"Adam B.", "Adam Bachouche", "T-002", 17, 1, 0, 1, 0x27272A, 0x18181B, P_SASSY},
};
static const int NCHAR = (int)(sizeof(ROSTER) / sizeof(ROSTER[0]));
enum { I_AMIR = 0, I_ADAM = 2, I_ARWA = 7, I_DALIA = 8, I_GABRIEL = 10, I_AMINE = 12, I_ALICIA = 17, I_ABDULLAH = 18, I_JALIL = 19, I_ANES = 22, I_AMYAS = 24, I_ADAMB = 25 };

// ---------------------------------------------------------------- rooms: one apartment per resident + shared spots
enum { ROOM_LOBBY = NCHAR, ROOM_ICE, ROOM_PARK, NROOMS };
static const char* sharedRoomName(int r) { return r == ROOM_LOBBY ? "Lobby" : r == ROOM_ICE ? "Ice Cream Shop" : "Parking 1 (outside)"; }

// ---------------------------------------------------------------- items
enum { CAT_FOOD, CAT_CLOTH, CAT_GOODS };
enum { SL_HAT, SL_TOP, SL_BOTTOM, SL_GLASSES };
struct ItemDef { const char* name; int cat; int price; int slot; uint32_t color; const char* tag; int power; };
static const ItemDef ITEMS[] = {
    // food (power = fullness restored)
    {"Makroud", CAT_FOOD, 120, 0, 0xD9A441, "MK", 30}, {"Mhadjeb", CAT_FOOD, 90, 0, 0xE0B25C, "MJ", 35}, {"Brik", CAT_FOOD, 110, 0, 0xE8C26D, "BK", 35},
    {"Couscous", CAT_FOOD, 220, 0, 0xF3D98B, "CC", 60}, {"Chorba", CAT_FOOD, 150, 0, 0xC2410C, "CH", 45}, {"Garantita", CAT_FOOD, 80, 0, 0xFACC15, "GR", 30},
    {"Ice Cream", CAT_FOOD, 100, 0, 0xF9A8D4, "IC", 25}, {"Pizza", CAT_FOOD, 250, 0, 0xEF4444, "PZ", 55}, {"Mint Tea", CAT_FOOD, 60, 0, 0x4ADE80, "TE", 15},
    {"Croissant", CAT_FOOD, 70, 0, 0xFBBF24, "CR", 25}, {"Dates", CAT_FOOD, 50, 0, 0x7C2D12, "DT", 20},
    // clothes
    {"Orange Cap", CAT_CLOTH, 150, SL_HAT, 0xF97316, "CP", 0}, {"Beanie", CAT_CLOTH, 130, SL_HAT, 0x334155, "BN", 0}, {"Crown", CAT_CLOTH, 600, SL_HAT, 0xFACC15, "CW", 0},
    {"Round Glasses", CAT_CLOTH, 180, SL_GLASSES, 0x111111, "RG", 0}, {"Sunglasses", CAT_CLOTH, 220, SL_GLASSES, 0x000000, "SG", 0},
    {"Orange Hoodie", CAT_CLOTH, 300, SL_TOP, 0xF97316, "OH", 0}, {"Black Jacket", CAT_CLOTH, 380, SL_TOP, 0x18181B, "BJ", 0}, {"Blue Tee", CAT_CLOTH, 160, SL_TOP, 0x3B82F6, "BT", 0},
    {"Green Tee", CAT_CLOTH, 160, SL_TOP, 0x22C55E, "GT", 0}, {"Cargo Pants", CAT_CLOTH, 260, SL_BOTTOM, 0x65634A, "CG", 0}, {"Blue Jeans", CAT_CLOTH, 240, SL_BOTTOM, 0x1D4ED8, "JN", 0},
    // goods
    {"Phone", CAT_GOODS, 500, 0, 0x94A3B8, "PH", 25}, {"Game Console", CAT_GOODS, 900, 0, 0xEF4444, "GC", 40}, {"Book", CAT_GOODS, 120, 0, 0x92400E, "BK", 15},
    {"Plush", CAT_GOODS, 200, 0, 0xFDA4AF, "PL", 30}, {"Balloon", CAT_GOODS, 60, 0, 0x38BDF8, "BL", 15}, {"Football", CAT_GOODS, 190, 0, 0xE5E7EB, "FB", 30},
    {"Headphones", CAT_GOODS, 350, 0, 0x1F2937, "HP", 30}, {"Camera", CAT_GOODS, 450, 0, 0x6B7280, "CM", 30}, {"Bot Polish", CAT_GOODS, 140, 0, 0xA3E635, "BP", 20},
    {"Ash Crystal", CAT_GOODS, 0, 0, 0x7C3AED, "AC", 10},   // cursed: never sold, only found
    {"Mystery Box", CAT_GOODS, 300, 0, 0x9333EA, "??", 20},
};
static const int NITEMS = (int)(sizeof(ITEMS) / sizeof(ITEMS[0]));
enum { IT_PHONE = 22, IT_CRYSTAL = 31, IT_MYSTERY = 32, IT_CROWN = 13, IT_ICE = 6 };
static const int GOODS_FAV[10] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 32};

// ---------------------------------------------------------------- dialogue
// {n} = other character's name.
static const char* L_GREET[] = {"Oh! Hi.", "You're here again.", "Yes? What is it?", "Hello! The building feels weird today.", "Hey, did you hear that humming?", "Hi hi!"};
static const char* L_CHAT[] = {"I had a weird dream about the elevator.", "My cleaning bot keeps judging me.", "Do the lights flicker on purpose?", "I'm a bit concerned about {n}'s snacks.",
                               "Have you ever counted the floors? Twice?", "The stairway door hums at night.", "I think the building is listening.", "Did you sleep okay? I didn't.",
                               "{n}, your shoes are untied.", "Wanna trade snacks later?"};
static const char* L_REPLY[] = {"Yeah... same.", "Haha, true.", "Hmm. I'll think about it.", "Don't say that so loud!", "Okay, but that's creepy.", "Honestly? Fair."};
static const char* L_THX_FOOD[] = {"Yum! Thank you!", "That hit the spot.", "Mmm, so good!", "I was starving, thanks!", "Delicious!"};
static const char* L_THX_ITEM[] = {"For me? Thank you!", "Whoa, nice!", "I'll take good care of it.", "This is great!", "You're the best!"};
static const char* L_THX_CLOTH[] = {"How do I look?", "Ooh, new look!", "It fits perfectly.", "Stylish, right?", "I feel different. Good different."};
static const char* L_FAVE[] = {"THIS IS MY FAVORITE!!", "How did you know?!", "I love it so much!"};
static const char* L_MEH[] = {"Um... thanks, I guess.", "Not really my thing.", "...okay."};
static const char* L_SAD[] = {"I feel kind of down...", "Nothing is fun today.", "Could use a little cheering up...", "Why is everything so quiet?"};
static const char* L_HUNGRY[] = {"My stomach is growling.", "I'm so hungry!", "Is there anything to eat?", "Food. Please."};
static const char* L_BORED[] = {"So bored...", "Wanna do something?", "Let's play something!", "Play with me!"};
static const char* L_WIN[] = {"That was awesome!", "Good game!", "Again sometime!", "I had so much fun!"};
static const char* L_LOSE[] = {"Aw, close!", "Next time!", "It's okay, that was fun anyway.", "Hmph. Rematch later."};
static const char* L_TINY_GREET[] = {"Hi hi hi!", "Look look!", "Up! Up!", "Snack?", "Boop!"};
static const char* L_TINY_CHAT[] = {"I saw a big bug.", "My bot is my friend.", "Lalala!", "Why is the sky orange?", "I'm a dinosaur."};
static const char* L_TINY_THX[] = {"Yaaay!!", "Mine mine!", "Thank youuu!", "Wheee!"};
static const char* L_PHONE_TXT[] = {"did the elevator just say my name", "come to the lobby", "send snacks pls", "my bot is judging me again", "the lights did it again", "gg", "found a weird door",
                                    "are you mad at me?", "ice cream later?", "don't look at the ceiling", "lol", "who unplugged the moon"};
struct Dream { const char* text; int item; };
static const Dream DREAMS[] = {
    {"floating above the island, eating giant makroud", 0}, {"an elevator with no buttons going up forever", -1}, {"the cleaning bot became king", IT_CROWN},
    {"a crystal growing out of the parking lot", IT_CRYSTAL}, {"football on the roof at midnight", 27}, {"a phone that texts the future", IT_PHONE},
    {"ice cream raining on the lobby", IT_ICE}, {"the lights singing in harmony", -1}, {"a door that breathes", -1}, {"hot mint tea in the snow", 8},
};
static const int NDREAMS = (int)(sizeof(DREAMS) / sizeof(DREAMS[0]));
static const char* CURSED_EVENTS[] = {
    "The lights flicker across the whole island.", "A cleaning bot beeps in a language nobody speaks.", "{n} stares at a wall for ten whole seconds.",
    "The elevator opened on a floor that isn't there.", "Everybody said 'hm' at the exact same time.", "{n} swears the stairway door is breathing.",
    "The moon blinked.", "Static on every phone for a second.", "{n} found an Ash Crystal under the couch.", "The island rotated 1 degree. Nobody noticed. Almost nobody.",
};
static const int NCURSED = (int)(sizeof(CURSED_EVENTS) / sizeof(CURSED_EVENTS[0]));

// personality voice wrappers
static const char* VOICE_PRE[P_COUNT][2] = {{"", ""}, {"um... ", "..."}, {"Hmph. ", "Tch. "}, {"", ""}, {"Hehe! ", ""}, {"Listen! ", "Hey! "}, {"Ooh! ", ""}, {"Pfft. ", "Obviously. "}, {"", "Eh, "}, {"", ""}};
static const char* VOICE_SUF[P_COUNT][2] = {{" Yay!", ""}, {"", " ...sorry."}, {" ...not that I care.", ""}, {"", ""}, {" Hehe!", " Boop."}, {" Got it?", ""}, {" Fascinating!", ""}, {" Whatever.", ""}, {" ...cool.", ""}, {"", ""}};

#include "game.hpp"

void game::addquest(
    const name &player,
    const std::string &type,
    float required_amount)
{
    quests_t quests_table(get_self(), get_self().value);
    auto player_iter = quests_table.find(player.value);

    quest temp_quest = {type, required_amount, 0.0f};

    if (player_iter == quests_table.end())
    {
        quests_table.emplace(get_self(), [&](auto& new_row)
        {
            new_row.player = player;
            new_row.quests = {temp_quest};
        });
    }
    else
    {
        quests_table.modify(player_iter, get_self(), [&](auto& row)
        {
            row.quests.push_back(temp_quest);
        });
    }
}

void game::cmpltquest(
    const name &player,
    uint32_t quest_index)
{
    quests_t quests_table(get_self(), get_self().value);
    auto player_iter = quests_table.require_find(player.value, "No such quest");
    
    check(player_iter->quests.size() > quest_index, "Index outside of scope");
    
    quest temp = player_iter->quests[quest_index];
    check(temp.current_amount >= temp.required_amount, "Quest conditions are not met");

    quests_table.modify(player_iter, get_self(), [&](auto& row){
        row.quests.erase(row.quests.begin() + quest_index);
    });
}

void game::update_quests(const name &player, const std::string &type, float update_amount, bool set)
{
    quests_t quests_table(get_self(), get_self().value);
    auto player_iter = quests_table.require_find(player.value, "No such quest");

    quests_table.modify(player_iter, get_self(), [&](auto& row){
        for (auto& quest : row.quests)
        {
            if (quest.type == type)
            {
                if (!set)
                {
                    quest.current_amount += update_amount;
                }
                else
                {
                    quest.current_amount = std::max(quest.current_amount, update_amount);
                }
            }
        }
    });
}

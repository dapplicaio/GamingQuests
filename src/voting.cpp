#include "game.hpp"

void game::createvoting(
    const name &player,
    const std::string &resource_name,
    const float &new_ratio)
{
  require_auth(player);

  const uint64_t key_id = stringToUint64(resource_name);
  resourcecost_t resourcecost_table(get_self(), get_self().value);
  auto resourcecost_table_itr = resourcecost_table.require_find(key_id, "Could not find selected resource name");

  changeration_t changeration_table(get_self(), get_self().value);
  const uint64_t new_voting_id = changeration_table.available_primary_key();

  changeration_table.emplace(player, [&](auto &new_row)
                             {
    new_row.voting_id = new_voting_id;
    new_row.resource_name = resource_name;
    new_row.new_ratio = new_ratio; });
}

void game::vote(
    const name &player,
    const uint64_t &voting_id)
{
  require_auth(player);

  balance_t balance_table(get_self(), get_self().value);
  resourcecost_t resourcecost_table(get_self(), get_self().value);
  changeration_t changeration_table(get_self(), get_self().value);

  auto balance_table_itr = balance_table.require_find(player.value, "You don't have staked tokens to vote");
  auto changeration_table_itr = changeration_table.require_find(voting_id, "Could not find selected voting id");
  auto resourcecost_table_itr = resourcecost_table.require_find(stringToUint64(changeration_table_itr->resource_name));

  const asset goal_votes = asset(100 * 10000, symbol("GAME", 4)); // 100.0000 GAME tokens to apply changes
  asset total_votes = asset(0, symbol("GAME", 4));

  for (const auto &map_itr : changeration_table_itr->voted)
    total_votes += map_itr.second;

  if (total_votes + balance_table_itr->quantity >= goal_votes)
  {
    resourcecost_table.modify(resourcecost_table_itr, get_self(), [&](auto &new_row)
                              { new_row.ratio = changeration_table_itr->new_ratio; });

    changeration_table.erase(changeration_table_itr);
  }
  else
  {
    changeration_table.modify(changeration_table_itr, get_self(), [&](auto &new_row)
                              { new_row.voted[player] = balance_table_itr->quantity; });
  }
}

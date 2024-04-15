#include "game.hpp"

void game::setratio(const std::string &resource, const float &ratio)
{
  require_auth(get_self());

  const uint64_t key_id = stringToUint64(resource);
  resourcecost_t resourcecost_table(get_self(), get_self().value);
  auto resourcecost_table_itr = resourcecost_table.find(key_id);

  if (resourcecost_table_itr == std::end(resourcecost_table))
  {
    resourcecost_table.emplace(get_self(), [&](auto &new_row)
                               {
      new_row.key_id = key_id;
      new_row.resource_name = resource;
      new_row.ratio = ratio; });
  }
  else
  {
    resourcecost_table.modify(resourcecost_table_itr, get_self(), [&](auto &new_row)
                              {
      new_row.resource_name = resource;
      new_row.ratio = ratio; });
  }
}

void game::swap(const name &owner, const std::string &resource, const float &amount2swap)
{
  require_auth(owner);

  resourcecost_t resourcecost_table(get_self(), get_self().value);
  auto resourcecost_table_itr = resourcecost_table.require_find(stringToUint64(resource), "Could not find resource cost config");

  const float ratio = resourcecost_table_itr->ratio;

  const float token_amount = amount2swap / ratio;
  const asset tokens2receive = asset(1000 * token_amount, symbol("WAX", 4)); // change to token you have deployed

  reduce_owner_resources_balance(owner, std::map<std::string, float>({{resource, amount2swap}}));
  tokens_transfer(owner, tokens2receive);
  update_quests(owner, "swap", tokens2receive.amount);
}
#include "game.hpp"

void game::upgradeitem(
    const name &owner,
    const uint64_t &item_to_upgrade,
    const uint8_t &next_level,
    const uint64_t &staked_at_farmingitem)
{
  require_auth(owner);

  const int32_t &time_now = current_time_point().sec_since_epoch();

  auto assets = atomicassets::get_assets(get_self());
  auto asset_itr = assets.require_find(item_to_upgrade, ("Could not find staked item[" + std::to_string(item_to_upgrade) + "]").c_str());

  staked_t staked_table(get_self(), owner.value);
  auto staked_table_itr = staked_table.require_find(staked_at_farmingitem, "Could not find staked farming item");

  check(std::find(std::begin(staked_table_itr->staked_items), std::end(staked_table_itr->staked_items), item_to_upgrade) != std::end(staked_table_itr->staked_items),
        "Item [" + std::to_string(item_to_upgrade) + "] is not staked at farming item");

  // claiming mined resources before upgrade

  std::map<std::string, uint32_t> stats = get_stats(owner);
  const uint8_t upgrade_percentage = 2 + stats["vitality"] / 10.0f;
  const std::pair<std::string, float> item_reward = claim_item(asset_itr, upgrade_percentage, time_now, stats);
  if (item_reward != std::pair<std::string, float>())
  {
    if (item_reward.second > 0)
    {
      increase_owner_resources_balance(owner, std::map<std::string, float>({item_reward}));
    }
  }
  // upgrading
  upgrade_item(asset_itr, upgrade_percentage, owner, next_level, time_now, stats);
  update_mining_power_lb(owner);
}

void game::upgrade_item(
    atomicassets::assets_t::const_iterator &assets_itr,
    const uint8_t &upgrade_percentage,
    const name &owner,
    const uint8_t &new_level,
    const uint32_t &time_now,
    const std::map<std::string, uint32_t> &stats)
{
  auto mdata = get_mdata(assets_itr);
  auto template_idata = get_template_idata(assets_itr->template_id, assets_itr->collection_name);

  const float &mining_rate = std::get<float>(template_idata["miningRate"]);
  const uint8_t &current_lvl = std::get<uint8_t>(mdata["level"]);
  const std::string &resource_name = std::get<std::string>(template_idata["farmResource"]);
  check(current_lvl < new_level, "New level must be higher then current level");
  check(new_level <= std::get<uint8_t>(template_idata["maxLevel"]), "New level can not be higher then max level");
  check(std::get<uint32_t>(mdata["lastClaim"]) < time_now, "Item is upgrading");

  float miningRate_according2lvl = mining_rate + stats.at("productivity") / 10.0f;
  for (uint8_t i = 1; i < new_level; ++i)
    miningRate_according2lvl = miningRate_according2lvl + (miningRate_according2lvl * upgrade_percentage / 100);

  const int32_t &upgrade_time = get_upgrading_time(new_level) - get_upgrading_time(current_lvl);
  const float &resource_price = upgrade_time * miningRate_according2lvl * (1.0f - stats.at("economic") / 100.0f);

  std::get<uint8_t>(mdata["level"]) = new_level;
  std::get<uint32_t>(mdata["lastClaim"]) = time_now + upgrade_time;

  reduce_owner_resources_balance(owner, std::map<std::string, float>({{resource_name, resource_price}}));

  update_mdata(assets_itr, mdata, get_self());
  update_quests(owner, "upgrade", new_level, true);
}

const int32_t game::get_upgrading_time(const uint8_t &end_level)
{
  const int32_t increasing_time = 320; // 5.33 min
  int32_t total_time = 0;
  int32_t temp_tracker = 0;
  for (uint8_t i = 2; i <= end_level; ++i)
  {
    if (i % 5 == 0)
    {
      total_time += (temp_tracker * 5);
    }
    else
    {
      temp_tracker += increasing_time;
      total_time += increasing_time;
    }
  }
  return total_time;
}

void game::upgfarmitem(const name &owner, const uint64_t &farmingitem_to_upgrade, const bool &staked)
{
  require_auth(owner);

  if (staked)
  {
    auto assets = atomicassets::get_assets(get_self());
    auto asset_itr = assets.require_find(farmingitem_to_upgrade, ("Could not find staked item[" + std::to_string(farmingitem_to_upgrade) + "]").c_str());

    staked_t staked_table(get_self(), owner.value);
    staked_table.require_find(farmingitem_to_upgrade, "Could not find staked farming item");

    upgrade_farmingitem(asset_itr, get_self());
  }
  else
  {
    auto assets = atomicassets::get_assets(owner);
    auto asset_itr = assets.require_find(farmingitem_to_upgrade, ("You do not own farmingitem[" + std::to_string(farmingitem_to_upgrade) + "]").c_str());
    upgrade_farmingitem(asset_itr, owner);
  }
}

void game::upgrade_farmingitem(atomicassets::assets_t::const_iterator &assets_itr, const name &owner)
{
  auto mdata = get_mdata(assets_itr);
  auto template_idata = get_template_idata(assets_itr->template_id, assets_itr->collection_name);

  check(std::get<uint8_t>(mdata["slots"])++ < std::get<uint8_t>(template_idata["maxSlots"]), "Farmingitem has max slots");

  update_mdata(assets_itr, mdata, owner);
}

#include "game.hpp"

void game::set_avatar(const name &owner, const uint64_t &asset_id)
{
  auto assets = atomicassets::get_assets(get_self());
  auto asset_itr = assets.find(asset_id);

  check(asset_itr->collection_name == "minersgamers"_n, "Wrong collection");
  check(asset_itr->schema_name == "avatar"_n, "Not an avatar asset");

  avatars_t avatars_table(get_self(), get_self().value);
  auto owner_avatar_itr = avatars_table.find(owner.value);

  if (owner_avatar_itr == std::end(avatars_table))
  {
    avatars_table.emplace(get_self(), [&](auto &new_row)
                          {
         new_row.owner = owner;
         new_row.equipment.resize(5);
         new_row.equipment[0] = asset_id; });
  }
  else
  {
    // should return avatar asset back to player
    const uint64_t old_avatar_id = owner_avatar_itr->equipment[0];

    const std::vector<uint64_t> assets_to_transfer = {old_avatar_id};
    const std::string memo = "return avatar";

    action(
        permission_level{get_self(), "active"_n},
        atomicassets::ATOMICASSETS_ACCOUNT,
        "transfer"_n,
        std::make_tuple(
            get_self(),
            owner,
            assets_to_transfer,
            memo))
        .send();

    avatars_table.modify(owner_avatar_itr, get_self(), [&](auto &row)
                         { row.equipment[0] = asset_id; });
  }

  recalculate_stats(owner);
  update_mining_power_lb(owner);
}

void game::set_equipment_item(const name &owner, const uint64_t asset_id, std::vector<uint64_t> &assets_to_return, std::map<std::string, uint32_t> &equiped_types)
{
  avatars_t avatars_table(get_self(), get_self().value);

  auto owner_avatar_itr = avatars_table.find(owner.value);
  check(owner_avatar_itr != std::end(avatars_table), "You can put equipment only when you have an avatar");

  auto assets = atomicassets::get_assets(get_self());
  auto asset_itr = assets.find(asset_id);
  auto equipment_template_idata = get_template_idata(asset_itr->template_id, asset_itr->collection_name);

  check(asset_itr->collection_name == "minersgamers"_n, "Wrong collection");
  check(asset_itr->schema_name == "equip"_n, "Not an equipment item");

  uint32_t position = 0;
  const std::string type = std::get<std::string>(equipment_template_idata["type"]);

  equiped_types[type]++;
  check(equiped_types[type] <= 1, "You can wear only 4 different euipment types at once");

  if (type == "flag")
  {
    position = 1;
  }
  else if (type == "jewelry")
  {
    position = 2;
  }
  else if (type == "crown")
  {
    position = 3;
  }
  else if (type == "cloak")
  {
    position = 4;
  }
  else
  {
    check(false, "Wrong type of equipment");
  }

  const uint64_t old_equip_id = owner_avatar_itr->equipment[position];

  if (old_equip_id != 0)
  {
    assets_to_return.push_back(old_equip_id);
  }

  avatars_table.modify(owner_avatar_itr, get_self(), [&](auto &row)
                       { row.equipment[position] = asset_id; });
}

void game::set_equipment_list(const name &owner, const std::vector<uint64_t> &asset_ids)
{
  std::vector<uint64_t> assets_to_return;

  std::map<std::string, uint32_t> equiped_types;
  equiped_types.insert(std::pair<std::string, uint32_t>("flag", 0));
  equiped_types.insert(std::pair<std::string, uint32_t>("jewelry", 0));
  equiped_types.insert(std::pair<std::string, uint32_t>("crown", 0));
  equiped_types.insert(std::pair<std::string, uint32_t>("cloak", 0));

  for (uint64_t asset_id : asset_ids)
  {
    set_equipment_item(owner, asset_id, assets_to_return, equiped_types);
  }

  const std::string memo = "return equipment";

  action(
      permission_level{get_self(), "active"_n},
      atomicassets::ATOMICASSETS_ACCOUNT,
      "transfer"_n,
      std::make_tuple(
          get_self(),
          owner,
          assets_to_return,
          memo))
      .send();

  recalculate_stats(owner);
  update_mining_power_lb(owner);
}

void game::recalculate_stats(const name &owner)
{
  stats_t stats_table(get_self(), get_self().value);
  auto stats_itr = stats_table.find(owner.value);

  std::map<std::string, uint32_t> stats;

  // init stats
  stats.insert(std::pair<std::string, uint32_t>("economic", 0));
  stats.insert(std::pair<std::string, uint32_t>("productivity", 0));
  stats.insert(std::pair<std::string, uint32_t>("vitality", 0));
  stats.insert(std::pair<std::string, uint32_t>("bravery", 0));
  stats.insert(std::pair<std::string, uint32_t>("diplomacy", 0));

  // read stats
  avatars_t avatars_table(get_self(), get_self().value);
  auto avatar_itr = avatars_table.require_find(owner.value, "Your avatar was deleted");

  auto assets = atomicassets::get_assets(get_self());

  for (uint64_t asset_id : avatar_itr->equipment)
  {
    if (asset_id == 0)
    {
      continue;
    }

    auto asset_itr = assets.find(asset_id);
    auto equipment_template_idata = get_template_idata(asset_itr->template_id, asset_itr->collection_name);

    for (auto &key_value_pair : stats)
    {
      if (equipment_template_idata.find(key_value_pair.first) != std::end(equipment_template_idata))
      {
        key_value_pair.second += std::get<uint32_t>(equipment_template_idata[key_value_pair.first]);
      }
    }
  }

  if (stats_itr == std::end(stats_table))
  {
    stats_table.emplace(get_self(), [&](auto &new_row)
                        {
      new_row.owner = owner;
      new_row.stats = stats; });
  }
  else
  {
    stats_table.modify(stats_itr, get_self(), [&](auto &row)
                       { row.stats = stats; });
  }
}

std::map<std::string, uint32_t> game::get_stats(const name &owner)
{
  std::map<std::string, uint32_t> stats;
  stats_t stats_table(get_self(), get_self().value);
  auto stats_itr = stats_table.find(owner.value);

  if (stats_itr == std::end(stats_table))
  {
    stats.insert(std::pair<std::string, uint32_t>("economic", 0));
    stats.insert(std::pair<std::string, uint32_t>("productivity", 0));
    stats.insert(std::pair<std::string, uint32_t>("vitality", 0));
    stats.insert(std::pair<std::string, uint32_t>("bravery", 0));
    stats.insert(std::pair<std::string, uint32_t>("diplomacy", 0));
  }
  else
  {
    stats = stats_itr->stats;
  }

  return stats;
}
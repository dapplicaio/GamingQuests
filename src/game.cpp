#include "game.hpp"

void game::receive_asset_transfer(
    const name &from,
    const name &to,
    std::vector<uint64_t> &asset_ids,
    const std::string &memo)
{
  if (to != get_self())
    return;

  if (memo == "stake farming item")
  {
    check(asset_ids.size() == 1, "You must transfer only one farming item to stake");
    stake_farmingitem(from, asset_ids[0]);
  }
  else if (memo.find("stake items:") != std::string::npos)
  {
    const uint64_t farmingitem_id = std::stoll(memo.substr(12));
    stake_items(from, farmingitem_id, asset_ids);
  }
  else if (memo.find("blend:") != std::string::npos)
  {
    const uint64_t blend_id = std::stoll(memo.substr(6));
    blend(from, asset_ids, blend_id);
  }
  else if (memo == "set avatar")
  {
    check(asset_ids.size() == 1, "You must transfer only one avatar");
    set_avatar(from, asset_ids[0]);
  }
  else if (memo == "set equipment")
  {
    check(asset_ids.size() <= 4, "You can wear only 4 different euipment types at once");
    set_equipment_list(from, asset_ids);
  }
  else
    check(0, "Invalid memo");
}

void game::receive_token_transfer(
    const name &from,
    const name &to,
    const asset &quantity,
    const std::string &memo)

{
  if (to != get_self())
    return;

  if (memo == "stake")
  {
    increase_tokens_balance(from, quantity);
  }
  else
    check(0, "Invalid memo");
}

void game::increase_tokens_balance(const name &owner, const asset &quantity)
{
  balance_t balance_table(get_self(), get_self().value);
  auto balance_table_itr = balance_table.find(owner.value);

  if (balance_table_itr == std::end(balance_table))
  {
    balance_table.emplace(get_self(), [&](auto &new_row)
                          {
      new_row.owner = owner;
      new_row.quantity = quantity; });
  }
  else
  {
    balance_table.modify(balance_table_itr, get_self(), [&](auto &new_row)
                         { new_row.quantity += quantity; });
  }

  set_lb_points("tokens"_n, owner, balance_table_itr->quantity.amount);
  update_quests(owner, "tokens", quantity.amount);
}

void game::tokens_transfer(const name &to, const asset &quantity)
{
  action(
      permission_level{get_self(), "active"_n},
      "eosio.token"_n, // change to your deployed token contract
      "transfer"_n,
      std::make_tuple(
          get_self(),
          to,
          quantity,
          std::string("")))
      .send();
}

void game::increase_owner_resources_balance(const name &owner, const std::map<std::string, float> &resources)
{
  resources_t resources_table(get_self(), owner.value);
  for (const auto &map_itr : resources)
  {
    const uint64_t &key_id = stringToUint64(map_itr.first);

    auto resources_table_itr = resources_table.find(key_id);
    if (resources_table_itr == std::end(resources_table))
    {
      resources_table.emplace(get_self(), [&](auto &new_row)
                              {
        new_row.key_id          = key_id;
        new_row.resource_name   = map_itr.first;
        new_row.amount          = map_itr.second; });
    }
    else
    {
      resources_table.modify(resources_table_itr, get_self(), [&](auto &new_row)
                             { new_row.amount += map_itr.second; });
    }

    eosio::name resource_name(static_cast<std::string_view>(resources_table_itr->resource_name));
    set_lb_points(resource_name, owner, resources_table_itr->amount);
  }
}

void game::reduce_owner_resources_balance(const name &owner, const std::map<std::string, float> &resources)
{
  resources_t resources_table(get_self(), owner.value);

  for (const auto &map_itr : resources)
  {
    const uint64_t &key_id = stringToUint64(map_itr.first);
    auto resources_table_itr = resources_table.require_find(key_id,
                                                            ("Could not find balance of " + map_itr.first).c_str());
    check(resources_table_itr->amount >= map_itr.second, ("Overdrawn balance: " + map_itr.first).c_str());

    if (resources_table_itr->amount == map_itr.second)
      resources_table.erase(resources_table_itr);
    else
    {
      resources_table.modify(resources_table_itr, get_self(), [&](auto &new_row)
                             { new_row.amount -= map_itr.second; });
    }

    eosio::name resource_name(static_cast<std::string_view>(resources_table_itr->resource_name));
    set_lb_points(resource_name, owner, resources_table_itr->amount);
  }
}

atomicassets::ATTRIBUTE_MAP game::get_mdata(atomicassets::assets_t::const_iterator &assets_itr)
{
  auto schemas = atomicassets::get_schemas(assets_itr->collection_name);
  auto schema_itr = schemas.find(assets_itr->schema_name.value);

  atomicassets::ATTRIBUTE_MAP deserialized_mdata = atomicdata::deserialize(
      assets_itr->mutable_serialized_data,
      schema_itr->format);

  return deserialized_mdata;
}

atomicassets::ATTRIBUTE_MAP game::get_template_idata(const int32_t &template_id, const name &collection_name)
{
  auto templates = atomicassets::get_templates(collection_name);
  auto template_itr = templates.find(template_id);

  auto schemas = atomicassets::get_schemas(collection_name);
  auto schema_itr = schemas.find(template_itr->schema_name.value);

  return atomicdata::deserialize(
      template_itr->immutable_serialized_data,
      schema_itr->format);
}

void game::update_mdata(atomicassets::assets_t::const_iterator &assets_itr, const atomicassets::ATTRIBUTE_MAP &new_mdata, const name &owner)
{
  action(
      permission_level{get_self(), "active"_n},
      atomicassets::ATOMICASSETS_ACCOUNT,
      "setassetdata"_n,
      std::make_tuple(
          get_self(),
          owner,
          assets_itr->asset_id,
          new_mdata))
      .send();
}

const uint64_t game::stringToUint64(const std::string &str)
{
  uint64_t hash = 0;

  if (str.size() == 0)
    return hash;

  for (int i = 0; i < str.size(); ++i)
  {
    int char_s = str[i];
    hash = ((hash << 4) - hash) + char_s;
    hash = hash & hash;
  }

  return hash;
}

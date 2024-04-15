#include "game.hpp"

void game::blend(const name &owner, const std::vector<uint64_t> asset_ids, const uint64_t &blend_id)
{
    auto assets = atomicassets::get_assets(get_self());
    auto templates = atomicassets::get_templates(get_self());
    blends_t blends_table(get_self(), get_self().value);
    auto blends_table_itr = blends_table.require_find(blend_id, "Could not find blend id");
    check(blends_table_itr->blend_components.size() == asset_ids.size(), "Blend components count mismatch");

    std::vector<int32_t> temp = blends_table_itr->blend_components;
    for (const uint64_t &asset_id : asset_ids)
    {
        auto assets_itr = assets.find(asset_id);
        check(assets_itr->collection_name == name("collname"), // replace collection with your collection name to check for fake nfts
              ("Collection of asset [" + std::to_string(asset_id) + "] mismatch").c_str());
        auto found = std::find(std::begin(temp), std::end(temp), assets_itr->template_id);
        if (found != std::end(temp))
            temp.erase(found);

        action(
            permission_level{get_self(), "active"_n},
            atomicassets::ATOMICASSETS_ACCOUNT,
            "burnasset"_n,
            std::make_tuple(
                get_self(),
                asset_id))
            .send();
    }
    check(temp.size() == 0, "Invalid blend components");

    auto templates_itr = templates.find(blends_table_itr->resulting_item);

    action(
        permission_level{get_self(), "active"_n},
        atomicassets::ATOMICASSETS_ACCOUNT,
        "mintasset"_n,
        std::make_tuple(
            get_self(),
            get_self(),
            templates_itr->schema_name,
            blends_table_itr->resulting_item,
            owner,
            (atomicassets::ATTRIBUTE_MAP){}, // immutable_data
            (atomicassets::ATTRIBUTE_MAP){}, // mutable data
            (std::vector<asset>){}           // token back
            ))
        .send();
}

void game::addblend(
    const std::vector<int32_t> blend_components,
    const int32_t resulting_item)
{
    require_auth(get_self());

    blends_t blends_table(get_self(), get_self().value);

    const uint64_t new_blend_id = blends_table.available_primary_key();

    blends_table.emplace(get_self(), [&](auto new_row)
                         {
    new_row.blend_id = new_blend_id;
    new_row.blend_components = blend_components;
    new_row.resulting_item = resulting_item; });
}
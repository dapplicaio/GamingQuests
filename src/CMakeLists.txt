project(game)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( game game quests.cpp config.cpp governance.cpp lboards.cpp game.cpp staking.cpp claim.cpp upgrade.cpp swap.cpp voting.cpp blend.cpp avatars.cpp)

target_include_directories( game PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( game ${CMAKE_SOURCE_DIR}/../ricardian )
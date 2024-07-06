#include "sekai/team_builder/neighbor_teams.h"

#include <array>
#include <span>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sekai/card.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/all.h"
#include "sekai/team.h"
#include "sekai/team_builder/constraints.h"

namespace sekai {
namespace {

using ::sekai::db::MasterDb;
using ::testing::UnorderedElementsAre;

TEST(SimpleNeighborsTest, GeneratesSingleCardMutationsNoRepeatCharactersOrCard) {
  CardState state;
  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(1), state},   // 0 - Ichika
      {MasterDb::FindFirst<db::Card>(2), state},   // 1 - Ichika
      {MasterDb::FindFirst<db::Card>(5), state},   // 2 - Saki
      {MasterDb::FindFirst<db::Card>(6), state},   // 3 - Saki
      {MasterDb::FindFirst<db::Card>(9), state},   // 4 - Honami
      {MasterDb::FindFirst<db::Card>(13), state},  // 5 - Shiho
      {MasterDb::FindFirst<db::Card>(65), state},  // 6 - Kanade
      {MasterDb::FindFirst<db::Card>(81), state},  // 7 - Miku
  };
  std::vector<const Card*> card_ptrs;
  for (const Card& card : cards) {
    card_ptrs.push_back(&card);
  }

  // Starting team:  0, 2, 4, 5, 7
  // Possible teams: 1, 2, 4, 5, 7
  //                 6, 2, 4, 5, 7
  //                 0, 3, 4, 5, 7
  //                 0, 6, 4, 5, 7
  //                 0, 2, 6, 5, 7
  //                 0, 2, 4, 6, 7
  //                 0, 2, 4, 5, 6

  std::vector<const Card*> starting_cards = {&cards[0], &cards[2], &cards[4], &cards[5], &cards[7]};
  Team starting_team{starting_cards};

  Constraints constraints;
  std::vector<Team> new_teams = SimpleNeighbors(card_ptrs, constraints).GetNeighbors(starting_team);
  std::vector<std::vector<int>> new_team_cards;
  for (const Team& team : new_teams) {
    new_team_cards.emplace_back();
    for (const Card* card : team.cards()) {
      new_team_cards.back().push_back(card->card_id());
    }
  }

  EXPECT_THAT(new_team_cards, UnorderedElementsAre(UnorderedElementsAre(2, 5, 9, 13, 81),   // ...
                                                   UnorderedElementsAre(65, 5, 9, 13, 81),  // ...
                                                   UnorderedElementsAre(1, 6, 9, 13, 81),   // ...
                                                   UnorderedElementsAre(1, 65, 9, 13, 81),  // ...
                                                   UnorderedElementsAre(1, 5, 65, 13, 81),  // ...
                                                   UnorderedElementsAre(1, 5, 9, 65, 81),   // ...
                                                   UnorderedElementsAre(1, 5, 9, 13, 65)));
}

TEST(SimpleNeighborsTest, RespectsLeadConstraints) {
  CardState state;
  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(1), state},   // 0 - Ichika
      {MasterDb::FindFirst<db::Card>(2), state},   // 1 - Ichika
      {MasterDb::FindFirst<db::Card>(5), state},   // 2 - Saki
      {MasterDb::FindFirst<db::Card>(6), state},   // 3 - Saki
      {MasterDb::FindFirst<db::Card>(9), state},   // 4 - Honami
      {MasterDb::FindFirst<db::Card>(13), state},  // 5 - Shiho
      {MasterDb::FindFirst<db::Card>(65), state},  // 6 - Kanade
      {MasterDb::FindFirst<db::Card>(81), state},  // 7 - Miku
  };
  std::vector<const Card*> card_ptrs;
  for (const Card& card : cards) {
    card_ptrs.push_back(&card);
  }

  // Starting team:  0, 2, 4, 5, 7
  // Possible teams: 1, 2, 4, 5, 7
  //                 6, 2, 4, 5, 7
  //                 0, 3, 4, 5, 7
  //                 0, 2, 6, 5, 7
  //                 0, 2, 4, 6, 7
  //                 0, 2, 4, 5, 6

  std::vector<const Card*> starting_cards = {&cards[0], &cards[2], &cards[4], &cards[5], &cards[7]};
  Team starting_team{starting_cards};

  Constraints constraints;
  constraints.AddLeadChar(cards[2].character_id());
  std::vector<Team> new_teams = SimpleNeighbors(card_ptrs, constraints).GetNeighbors(starting_team);
  std::vector<std::vector<int>> new_team_cards;
  for (const Team& team : new_teams) {
    new_team_cards.emplace_back();
    for (const Card* card : team.cards()) {
      new_team_cards.back().push_back(card->card_id());
    }
  }

  EXPECT_THAT(new_team_cards, UnorderedElementsAre(UnorderedElementsAre(2, 5, 9, 13, 81),   // ...
                                                   UnorderedElementsAre(65, 5, 9, 13, 81),  // ...
                                                   UnorderedElementsAre(1, 6, 9, 13, 81),   // ...
                                                   UnorderedElementsAre(1, 5, 65, 13, 81),  // ...
                                                   UnorderedElementsAre(1, 5, 9, 65, 81),   // ...
                                                   UnorderedElementsAre(1, 5, 9, 13, 65)));
}

TEST(SimpleNeighborsTest, RespectsKizunaConstraints) {
  CardState state;
  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(1), state},   // 0 - Ichika
      {MasterDb::FindFirst<db::Card>(2), state},   // 1 - Ichika
      {MasterDb::FindFirst<db::Card>(5), state},   // 2 - Saki
      {MasterDb::FindFirst<db::Card>(6), state},   // 3 - Saki
      {MasterDb::FindFirst<db::Card>(9), state},   // 4 - Honami
      {MasterDb::FindFirst<db::Card>(13), state},  // 5 - Shiho
      {MasterDb::FindFirst<db::Card>(65), state},  // 6 - Kanade
      {MasterDb::FindFirst<db::Card>(81), state},  // 7 - Miku
  };
  std::vector<const Card*> card_ptrs;
  for (const Card& card : cards) {
    card_ptrs.push_back(&card);
  }

  // Starting team:  0, 2, 4, 5, 7
  // Possible teams: 1, 2, 4, 5, 7
  //                 0, 3, 4, 5, 7
  //                 0, 2, 4, 6, 7
  //                 0, 2, 4, 5, 6

  std::vector<const Card*> starting_cards = {&cards[0], &cards[2], &cards[4], &cards[5], &cards[7]};
  Team starting_team{starting_cards};

  Constraints constraints;
  constraints.AddKizunaPair({cards[0].character_id(), cards[2].character_id()});
  std::vector<Team> new_teams = SimpleNeighbors(card_ptrs, constraints).GetNeighbors(starting_team);
  std::vector<std::vector<int>> new_team_cards;
  for (const Team& team : new_teams) {
    new_team_cards.emplace_back();
    for (const Card* card : team.cards()) {
      new_team_cards.back().push_back(card->card_id());
    }
  }

  EXPECT_THAT(new_team_cards, UnorderedElementsAre(UnorderedElementsAre(2, 5, 9, 13, 81),
                                                   UnorderedElementsAre(1, 6, 9, 13, 81),
                                                   UnorderedElementsAre(1, 5, 65, 13, 81),
                                                   UnorderedElementsAre(1, 5, 9, 65, 81),
                                                   UnorderedElementsAre(1, 5, 9, 13, 65)));
}

TEST(SimpleNeighborsTest, RespectsLeadSkillConstraints) {
  CardState state;
  state.set_skill_level(4);
  std::vector<Card> cards = {
      {MasterDb::FindFirst<db::Card>(495), state}, {MasterDb::FindFirst<db::Card>(535), state},
      {MasterDb::FindFirst<db::Card>(259), state}, {MasterDb::FindFirst<db::Card>(622), state},
      {MasterDb::FindFirst<db::Card>(14), state},  {MasterDb::FindFirst<db::Card>(156), state},
      {MasterDb::FindFirst<db::Card>(224), state},  // Minori
  };
  std::vector<const Card*> card_ptrs;
  for (const Card& card : cards) {
    card_ptrs.push_back(&card);
  }

  // Starting team:  0, 1, 2, 3, 4
  // Possible teams: 5, 1, 2, 3, 4
  //                 0, 5, 2, 3, 4
  //                 0, 1, 5, 3, 4
  //                 0, 1, 2, 3, 5

  std::vector<const Card*> starting_cards = {&cards[0], &cards[1], &cards[2], &cards[3], &cards[4]};
  Team starting_team{starting_cards};

  Constraints constraints;
  constraints.SetMinLeadSkill(150);
  std::vector<Team> new_teams = SimpleNeighbors(card_ptrs, constraints).GetNeighbors(starting_team);
  std::vector<std::vector<int>> new_team_cards;
  for (const Team& team : new_teams) {
    new_team_cards.emplace_back();
    for (const Card* card : team.cards()) {
      new_team_cards.back().push_back(card->card_id());
    }
  }

  EXPECT_THAT(new_team_cards,
              UnorderedElementsAre(UnorderedElementsAre(156, 535, 259, 622, 14),  // ...
                                   UnorderedElementsAre(495, 156, 259, 622, 14),  // ...
                                   UnorderedElementsAre(495, 535, 156, 622, 14),  // ...
                                   UnorderedElementsAre(495, 535, 259, 622, 156)));
}

}  // namespace
}  // namespace sekai

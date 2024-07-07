#include "frontend/element_id.h"

#include <string>

#include "absl/strings/str_cat.h"

namespace frontend {

std::string CardListRowId(int card_id) { return absl::StrCat("card-list-card-", card_id); }

}  // namespace frontend

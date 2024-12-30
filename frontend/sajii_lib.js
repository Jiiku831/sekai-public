var controller = null;

const kMaxSupportTeamSize = 20;

function InitPage () {
  CreateTeamBuilder(0);
  CreateTeamBuilder(1);
  CreateTeamBuilder(2);
  controller = new Module.Controller();
  console.log("Ready.");
}

function CreateCheckbox(elem_id, cls, displayText, onchange, initial = true, disable = false, type = "checkbox", name = null, value = null) {
  const node = document.createElement("span");
  const checkbox = document.createElement("input");
  checkbox.classList.add(...cls);
  checkbox.disabled = disable;
  checkbox.type = type;
  checkbox.id = elem_id;
  checkbox.checked = initial;
  checkbox.addEventListener("change", onchange);
  if (name) {
    checkbox.name = name;
  }
  if (value) {
    checkbox.value = value;
  }
  node.appendChild(checkbox);
  if (displayText == "") {
    return node;
  }
  const label = document.createElement("label");
  label.htmlFor = checkbox.id;
  label.appendChild(document.createTextNode(displayText));
  node.appendChild(label);
  return node;
}

function CreateRadio(elem_id, name, value, cls, displayText, onchange, initial = false, disable = false) {
  return CreateCheckbox(elem_id, cls, displayText, onchange, initial, disable, "radio", name, value);
}

function CreateAttrFilters(attrs) {
  const node = document.getElementById("card-list-attr-filter");
  attrs.forEach((attr, index) => {
    node.appendChild(CreateCheckbox(
      `card-list-attr-filter-${index}`,
      ['attr-filter-checkbox'],
      attr.displayText,
      function (e) {
        controller.SetAttrFilterState(attr.value, this.checked);
      }));
  });
}

function CreateCharacterFilters(charRows) {
  const node = document.getElementById("card-list-char-filter");
  charRows.forEach((row, index1) => {
    const tr = document.createElement("tr");
    row.chars.forEach((character, index2) => {
      const cell = document.createElement("td");
      cell.appendChild(CreateCheckbox(
        `card-list-character-filter-${index1}-${index2}`,
        ['character-filter-checkbox'],
        character.displayText,
        function (e) {
          controller.SetCharacterFilterState(character.charId, this.checked);
        }));
      tr.appendChild(cell);
    });
    node.appendChild(tr);
  });
}

function CreateRarityFilters(rarities) {
  const node = document.getElementById("card-list-rarity-filter");
  rarities.forEach((rarity, index) => {
    node.appendChild(CreateCheckbox(
      `card-list-rarity-filter-${index}`,
      ['rarity-filter-checkbox'],
      rarity.displayText,
      function (e) {
        controller.SetRarityFilterState(rarity.value, this.checked);
      }));
  });
}

function CreateListFilters(filter) {
  CreateAttrFilters(filter.attrs);
  CreateCharacterFilters(filter.charRows);
  CreateRarityFilters(filter.rarities);
}

function CreateNode(tag, node) {
  const cell = document.createElement(tag);
  cell.appendChild(node);
  return cell;
}

function CreateSvgImage(src, res, defer = false) {
  const img = document.createElementNS("http://www.w3.org/2000/svg", "image");
  img.setAttribute("width", res);
  img.setAttribute("height", res);
  img.setAttribute("x", 0);
  img.setAttribute("y", 0);
  img.setAttribute(defer ? "data-href" : "href", src);
  return img;
}

function CreateCardThumbSvg(thumbnailRes, thumbnailUrl, rarityFrame, attrFrame, defer = false) {
  const img = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  img.setAttributeNS("http://www.w3.org/2000/xmlns/", "xmlns:xlink", "http://www.w3.org/1999/xlink");
  img.setAttribute("viewBox", `0 0 ${thumbnailRes} ${thumbnailRes}`);
  img.setAttribute("width", thumbnailRes);
  img.setAttribute("height", thumbnailRes);
  img.classList.add("card-thumbnail");
  img.appendChild(CreateSvgImage(thumbnailUrl, thumbnailRes, defer));
  if (rarityFrame) {
    img.appendChild(CreateSvgImage(rarityFrame, thumbnailRes, defer));
  }
  if (attrFrame) {
    img.appendChild(CreateSvgImage(attrFrame, thumbnailRes, defer));
  }
  return img;
}

function CreateCardThumb(card, useAlt = false) {
  const thumbnailUrl = useAlt ? card.altThumbnailUrl : card.thumbnailUrl;
  const rarityFrame = useAlt ? card.altRarityFrame : card.rarityFrame;
  const attrFrame = card.attrFrame;
  return CreateCardThumbSvg(card.thumbnailRes, thumbnailUrl, rarityFrame, attrFrame);
}

function CreateCardThumb128(card, useAlt = false) {
  const thumbnailUrl = useAlt ? card.altThumbnailUrl128 : card.thumbnailUrl128;
  const rarityFrame = useAlt ? card.altRarityFrame : card.rarityFrame;
  const attrFrame = card.attrFrame;
  return CreateCardThumbSvg(128, thumbnailUrl, rarityFrame, attrFrame, /*defer=*/true);
}

function CreateCardThumbHover(card, useAlt = false) {
  const node = document.createElement("div");
  node.classList.add("card-hover-container");
  node.appendChild(CreateCardThumb(card, useAlt));

  const child = document.createElement("div");
  child.classList.add("card-hover-child");
  child.appendChild(CreateCardThumb128(card, useAlt));
  node.appendChild(child);
  node.addEventListener("mouseover", () => {
    node.querySelectorAll(":scope image").forEach((e) => {
      if (e.hasAttribute("data-href")) {
        e.setAttribute("href", e.getAttribute("data-href"));
        e.removeAttribute("data-href");
      }
    });
  });
  return node;
}

function CreateCardList(list) {
  CreateListFilters(list.listFilter);
  const cardList = document.getElementById("card-list-body");
  list.cards.forEach((card) => {
    const img = CreateCardThumbHover(card);
    const altImg = CreateCardThumbHover(card, true);
    const row = document.createElement("tr");
    row.id = card.cardListRowId
    row.appendChild(
      CreateNode("td",
        CreateCheckbox(
          `card-list-owned-${card.cardId}`,
          ['card-owned-checkbox'],
          "",
          function (e) {
            controller.SetCardOwned(card.cardId, this.checked);
          },
          false)));
    row.appendChild(CreateNode("td", img));
    row.appendChild(CreateNode("td", card.trainable ? altImg : document.createTextNode("")));
    row.appendChild(CreateNode("td", document.createTextNode(card.cardId)));
    row.appendChild(CreateNode("td", document.createTextNode(card.character.displayText)));
    row.appendChild(CreateNode("td", document.createTextNode(card.attr.displayText)));
    row.appendChild(CreateNode("td", document.createTextNode(card.rarity.displayText)));
    row.appendChild(
      CreateNode("td",
        CreateNumberInput(1, card.maxLevel, `card-list-level-${card.cardId}`,
          function (e) {
            if (!this.validity.valid || this.value == "") {
              return;
            }
            controller.SetCardLevel(card.cardId, parseInt(this.value));
          },
          true)));
    row.appendChild(
      CreateNode("td",
        card.trainable
          ? CreateCheckbox(
            `card-list-trained-${card.cardId}`,
            ['card-trained-checkbox'],
            "",
            function (e) {
              controller.SetCardTrained(card.cardId, this.checked);
            },
            false,
            true)
          : document.createTextNode("")));
    row.appendChild(
      CreateNode("td",
        card.cardEpisode1
          ? CreateCheckbox(
            `card-list-episode-1-${card.cardId}`,
            ['card-episode-1-checkbox'],
            "",
            function (e) {
              controller.SetCardEpisodeRead(card.cardId, card.cardEpisode1, this.checked);
            },
            false,
            true)
          : document.createTextNode("")));
    row.appendChild(
      CreateNode("td",
        card.cardEpisode2
          ? CreateCheckbox(
            `card-list-episode-2-${card.cardId}`,
            ['card-episode-2-checkbox'],
            "",
            function (e) {
              controller.SetCardEpisodeRead(card.cardId, card.cardEpisode2, this.checked);
            },
            false,
            true)
          : document.createTextNode("")));
    row.appendChild(
      CreateNode("td",
        CreateNumberInput(0, 5, `card-list-master-rank-${card.cardId}`,
          function (e) {
            if (!this.validity.valid || this.value == "") {
              return;
            }
            controller.SetCardMasterRank(card.cardId, parseInt(this.value));
          },
          true)));
    row.appendChild(
      CreateNode("td",
        CreateNumberInput(1, 4, `card-list-skill-level-${card.cardId}`,
          function (e) {
            if (!this.validity.valid || this.value == "") {
              return;
            }
            controller.SetCardSkillLevel(card.cardId, parseInt(this.value));
          },
          true)));
    cardList.appendChild(row);
  });
}

function CreateNumberInput(min, max, elemId, onchange, disable = false) {
  const node = document.createElement("span");
  const input = document.createElement("input");
  input.disabled = disable;
  input.type = "number";
  input.min = min;
  input.max = max;
  input.value = min;
  input.id = elemId;
  input.addEventListener("change", onchange);
  const minButton = document.createElement("input");
  minButton.tabIndex = -1;
  minButton.disabled = disable;
  minButton.type = "button";
  minButton.value = "<<";
  minButton.addEventListener("click", function(e) {
    const elem = document.getElementById(elemId);
    elem.value = min;
    elem.dispatchEvent(new Event('change'));
  });
  const maxButton = document.createElement("input");
  maxButton.tabIndex = -1;
  maxButton.disabled = disable;
  maxButton.type = "button";
  maxButton.value = ">>";
  maxButton.addEventListener("click", function(e) {
    const elem = document.getElementById(elemId);
    elem.value = max;
    elem.dispatchEvent(new Event('change'));
  });

  node.appendChild(minButton);
  node.appendChild(input);
  node.appendChild(maxButton);
  return node;
}

function CreateBonusPowerRow(tbody, items, padding, createCell) {
  const nameRow = document.createElement("tr");
  const inputRow = document.createElement("tr");
  items.forEach((item) => {
    nameRow.appendChild(
      CreateNode("td", document.createTextNode(item.displayText)));
    inputRow.appendChild(createCell(item));
  });
  for (let i = 0; i < padding; ++i) {
    nameRow.appendChild(document.createElement("td"));
    inputRow.appendChild(document.createElement("td"));
  }
  tbody.appendChild(nameRow);
  tbody.appendChild(inputRow);
}

function CreateAreaItemRow(tbody, items, padding) {
  CreateBonusPowerRow(tbody, items, padding, (item) => {
    return CreateNode("td",
      CreateNumberInput(
          0, 15, `area-item-${item.areaItemId}`,
          function(e) {
              if (!this.validity.valid || this.value == "") {
                  return;
              }
              controller.SetAreaItemLevel(item.areaItemId, parseInt(this.value));
          }));
  });
}

function CreateCharacterRankRow(tbody, items, padding) {
  CreateBonusPowerRow(tbody, items, padding, (item) => {
    return CreateNode("td",
      CreateNumberInput(
          1, 160, `character-rank-${item.charId}`,
          function(e) {
              if (!this.validity.valid || this.value == "") {
                  return;
              }
              controller.SetCharacterRank(item.charId, parseInt(this.value));
          }));
  });
}

function CreatePowerBonusAreaItemRow(area, chunkSize) {
  const tbody = document.getElementById("power-bonus-table-body");
  const headerCell = CreateNode("th", document.createTextNode(area.displayText));
  headerCell.colSpan = chunkSize;
  tbody.appendChild(CreateNode("tr", headerCell));
  for (let i = 0; i < area.areaItems.length; i += chunkSize) {
    CreateAreaItemRow(tbody, area.areaItems.slice(i, i + chunkSize),
      i + chunkSize - area.areaItems.length);
  }
}

function CreateCharacterRankRows(rows, chunkSize) {
  const tbody = document.getElementById("power-bonus-table-body");
  const headerCell = CreateNode("th", document.createTextNode("Character Ranks"));
  headerCell.colSpan = chunkSize;
  tbody.appendChild(CreateNode("tr", headerCell));
  rows.forEach((row) => {
    for (let i = 0; i < row.chars.length; i += chunkSize) {
      CreateCharacterRankRow(tbody, row.chars.slice(i, i + chunkSize),
        i + chunkSize - row.chars.length);
    }
  });
}

function CreateTitleBonusRow(chunkSize) {
  const tbody = document.getElementById("power-bonus-table-body");
  const headerCell = CreateNode("th", document.createTextNode("Title Bonus"));
  headerCell.colSpan = chunkSize;
  tbody.appendChild(CreateNode("tr", headerCell));
  Array.from([[{displayText: "Title Bonus"}]]).forEach((row) => {
    CreateBonusPowerRow(tbody, row, chunkSize - 1, (item) => {
      return CreateNode("td",
        CreateNumberInput(
            0, 300, 'title-bonus',
            function(e) {
                if (!this.validity.valid || this.value == "") {
                    return;
                }
                controller.SetTitleBonus(parseInt(this.value));
            }));
    });
  });
}

function CreatePowerBonusTable(context) {
  const chunkSize = 5;
  context.areas.forEach((area) => {
    CreatePowerBonusAreaItemRow(area, chunkSize);
  });
  CreateCharacterRankRows(context.charRows, chunkSize);
  CreateTitleBonusRow(chunkSize);
}

function CreateEventBonusEventSelection(context, container) {
  const option = CreateNode("option", document.createTextNode(context.displayText));
  option.value = context.eventStr;
  option.dataset.eventId = context.eventId ?? 0;
  option.dataset.chapterId = context.chapterId ?? 0;
  option.selected = context.selected;
  container.appendChild(option);
}

function CreateEventBonusEventDropdown(events) {
  const event_select = document.getElementById("event-bonus-event-select");
  Array.from(events).forEach((event_context) => {
      CreateEventBonusEventSelection(event_context, event_select);
    });
}

function CreateChallengeLiveCharacterRow(chars, container) {
  const row = document.createElement("tr");
  Array.from(chars).forEach((character) => {
    const cell = document.createElement("td");
    cell.appendChild(
      CreateRadio(`cl-character-${character.charId}`, "cl-character-radio",
        character.charId, "", character.displayText, () => {}));
    row.appendChild(cell);
  });
  container.appendChild(row);
}

function CreateLeadConstraintsCheckbox(chars, container) {
  const row = document.createElement("tr");
  Array.from(chars).forEach((character) => {
    const cell = document.createElement("td");
    cell.appendChild(
      CreateCheckbox(`lead-constraint-${character.charId}`, "",
        character.displayText, (e) => {
          controller.SetLeadConstraint(character.charId, e.target.checked);
        }));
    row.appendChild(cell);
  });
  container.appendChild(row);
}

function CreateRarityConstraintsCheckbox(rarities, container) {
  Array.from(rarities).forEach((rarity) => {
    container.appendChild(
      CreateCheckbox(`rarity-constraint-${rarity.value}`, "",
        rarity.displayText, (e) => {
          controller.SetRarityConstraint(rarity.value, e.target.checked);
        }));
  });
}

function CreateKizunaTableHeaderRow(chars) {
  const row = document.createElement("tr");
  row.classList.add("kizuna-table-header");
  row.appendChild(CreateNode("th", document.createTextNode("")));
  Array.from(chars).forEach((c) => {
    const header = CreateNode("th", document.createTextNode(c.displayText));
    header.setAttribute("data-char2", c.charId);
    row.appendChild(header);
  });
  return row;
}

function CreateKizunaTableRow(mainChar, chars) {
  const row = document.createElement("tr");
  const header = CreateNode("th", document.createTextNode(mainChar.displayText));
  header.setAttribute("data-char1", mainChar.charId);
  header.classList.add("kizuna-table-char-name");
  row.appendChild(header);
  Array.from(chars).forEach((c) => {
    const cell = CreateNode("td",
      mainChar.charId == c.charId
        ? document.createTextNode("")
        : CreateCheckbox(
          `kizuna-constraint-${mainChar.charId}-${c.charId}`,
          "", "",
          (e) => {
            document.getElementById(`kizuna-constraint-${c.charId}-${mainChar.charId}`).checked
              = e.target.checked;
            controller.SetKizunaConstraint(c.charId, mainChar.charId, e.target.checked);
          }));
    cell.setAttribute("data-char1", mainChar.charId);
    cell.setAttribute("data-char2", c.charId);
    cell.addEventListener("mouseover", function (e) {
      document.querySelectorAll(".kizuna-table-highlight").forEach((e) => {
        e.classList.remove("kizuna-table-highlight");
      });
      document.querySelectorAll(`[data-char1="${mainChar.charId}"]`).forEach((e) => {
        e.classList.add("kizuna-table-highlight");
      });
      document.querySelectorAll(`[data-char2="${c.charId}"]`).forEach((e) => {
        e.classList.add("kizuna-table-highlight");
      });
    });
    row.appendChild(cell);
  });
  return row;
}

function CreateKizunaTable(context) {
  const table = document.createElement("table");
  table.classList.add("kizuna-table");
  table.appendChild(CreateKizunaTableHeaderRow(context.kizunaConstraint[0].chars));
  Array.from(context.kizunaConstraint).forEach((kizuna) => {
    table.appendChild(CreateKizunaTableRow(kizuna.kizunaMain, kizuna.chars));
  });
  document.getElementById("kizuna-constraints").appendChild(table);
}

function CreateCustomEventAttrs(attrs) {
  const container = document.getElementById("custom-event-attr");
  container.appendChild(
    CreateRadio(`custom-event-attr-0`,
      "custom-event-attr-radio", 0, "", "None", () => {
      controller.SetCustomEventAttr(0);
    }));
  Array.from(attrs).forEach((attr) => {
    container.appendChild(
      CreateRadio(`custom-event-attr-${attr.value}`, "custom-event-attr-radio",
        attr.value, "", attr.displayText, () => {
          controller.SetCustomEventAttr(attr.value);
          controller.SetCustomEventChapter(0);
          document.getElementById("custom-event-chapter-0").checked = true;
        }));
  });
}

function CreateCustomEventCharRow(index, chars, padding) {
  const row = document.createElement("tr");
  row.id = `custom-event-character-row-${index}`
  const unit = chars[0].unitDisplayText;
  row.appendChild(CreateNode("th",
    document.createTextNode(unit)));
  Array.from(chars).forEach((character) => {
    const unitId = character.unitId ?? 0;
    row.appendChild(
      CreateNode("td",
      CreateCheckbox(`custom-event-character-${character.charId}-${unitId}`, "", character.displayText, (e) => {
        controller.SetCustomEventCharacter(character.charId, unitId, e.target.checked);
      }), false));
  });
  for (let i = 0; i < padding; ++i) {
    row.appendChild(document.createElement("td"));
  }
  const allBtn = document.createElement("input");
  allBtn.type = "button";
  allBtn.value = "All";
  allBtn.addEventListener("click", () => {
    SetAllFilter(row.id, true);
  });
  row.appendChild(CreateNode("td", allBtn));
  const noneBtn = document.createElement("input");
  noneBtn.type = "button";
  noneBtn.value = "None";
  noneBtn.addEventListener("click", () => {
    SetAllFilter(row.id, false);
  });
  row.appendChild(CreateNode("td", noneBtn));
  return row;
}

function CreateCustomEventChars(groups) {
  const container = document.getElementById("custom-event-chars");
  const table = document.createElement("table");
  table.classList.add("custom-event-chars-table");
  const firstRowLength = groups[0].chars.length;
  for (let i = 0; i < groups.length; ++i) {
    table.appendChild(CreateCustomEventCharRow(i, groups[i].chars, firstRowLength - groups[i].chars.length));
  }
  container.appendChild(table);
}

function CreateCustomEventChapterRow(chars, container) {
  const row = document.createElement("tr");
  Array.from(chars).forEach((character) => {
    const cell = document.createElement("td");
    cell.appendChild(
      CreateRadio(`custom-event-chapter-${character.charId}`,
        "custom-event-chapter-radio", character.charId, "", character.displayText, () => {
        controller.SetCustomEventChapter(character.charId);
        controller.SetCustomEventAttr(0);
        document.getElementById("custom-event-attr-0").checked = true;
      }));
    row.appendChild(cell);
  });
  container.appendChild(row);
}

function CreateCustomEventChapters(groups) {
  const container = document.getElementById("custom-event-chapter");
  container.appendChild(
    CreateRadio(`custom-event-chapter-0`,
      "custom-event-chapter-radio", 0, "", "None", () => {
      controller.SetCustomEventChapter(0);
    }));
  container.appendChild(document.createElement("br"));
  Array.from(groups).forEach((group) => {
    CreateCustomEventChapterRow(group.chars, container);
  });
  container.appendChild(document.createElement("br"));
}

function CreateCustomEventWorldBloomVersions(versions) {
  const container = document.getElementById("custom-event-wl-version");
  Array.from(versions).forEach((version) => {
    container.appendChild(
      CreateRadio(`custom-event-wl-version-${version.value}`, "custom-event-wl-version-radio",
        version.value, "", version.displayText, () => {
          controller.SetCustomEventWorldBloomVersion(version.value);
        }));
  });
}

function CreateCustomEvent(context) {
  CreateCustomEventAttrs(context.attrs);
  CreateCustomEventChars(context.bonusChars);
  CreateCustomEventChapters(context.chapterChars);
  CreateCustomEventWorldBloomVersions(context.worldBloomVersions);
}

function CreateTeamRecommender(context) {
  CreateCustomEvent(context.customEvent);
  CreateEventBonusEventDropdown(context.events);
  const clCharContainer = document.getElementById("cl-character-select");
  Array.from(context.challengeLiveCharacters).forEach((group) => {
    CreateChallengeLiveCharacterRow(group.chars, clCharContainer);
  });

  document.getElementById("use-old-subunitless-bonus-container").appendChild(
    CreateCheckbox("use-old-subunitless-bonus", "",
      "Use pre-bloom fes subunitless VS bonus",
      (e) => {
        controller.SetUseOldSubunitlessBonus(e.target.checked);
      }, context.useOldSubunitlessBonus));

  const leadCharContainer = document.getElementById("lead-constraints");
  Array.from(context.leadCharacterConstraint).forEach((group) => {
    CreateLeadConstraintsCheckbox(group.chars, leadCharContainer);
  });
  CreateKizunaTable(context);

  const raritiesContainer = document.getElementById("rarity-constraints");
  CreateRarityConstraintsCheckbox(context.rarityConstraint, raritiesContainer);
}

function InitialRender(context) {
  CreatePowerBonusTable(context.powerBonus);
  CreateCardList(context.cardList);
  CreateTeamRecommender(context.teamBuilder);
}

function SetAllFilter(elem, val) {
  Array.from(document.querySelectorAll(`#${elem} input`)).forEach((e) => {
    e.checked = val;
    e.dispatchEvent(new Event('change'));
  });
}

function SetEventBonusByEvent(elem) {
  controller.SetEventBonusByEvent(
    parseInt(elem.selectedOptions[0].dataset.eventId),
    parseInt(elem.selectedOptions[0].dataset.chapterId));
}

function SetVisibleOwned(state) {
  Array.from(
    document.querySelectorAll("#card-list-body tr:not(.hidden) input.card-owned-checkbox"))
  .forEach((e) => {
    e.checked = state;
    e.dispatchEvent(new Event('change'));
  });
}

function CreateTeamBuilder(index) {
  const teamHeader = CreateNode("th", document.createTextNode(`Team ${index}`));
  teamHeader.rowSpan = 3;
  const inputRow = document.createElement("tr");
  inputRow.classList.add("team-builder-id-row");
  inputRow.appendChild(teamHeader);
  const thumbRow = document.createElement("tr");
  thumbRow.classList.add("team-builder-thumb-row");
  const statsRow = document.createElement("tr");
  statsRow.classList.add("team-builder-stats-row");
  for (let i = 0; i < 5; ++i) {
    const thumbCell = document.createElement("td");
    thumbCell.colSpan = 2;
    thumbCell.id = `team-builder-${index}-${i}-thumb`;
    thumbRow.appendChild(thumbCell);

    const input = document.createElement("input");
    input.id = `team-builder-${index}-${i}-id`;
    input.type = "number";
    input.min = 0;
    input.max = 9999;
    input.classList.add("team-builder-id");
    input.placeholder = "Enter card ID";
    input.addEventListener("change", function (e) {
      this.setCustomValidity("");
      let isValid = true;
      if (!this.validity.valid || this.value == "") {
        isValid = false;
      } else if (!controller.IsValidCard(parseInt(this.value))) {
        this.setCustomValidity("Invalid card ID.");
        isValid = false;
      }
      if (isValid) {
        controller.SetTeamCard(index, i, parseInt(this.value));
      } else {
        controller.ClearTeamCard(index, i);
      }
    });
    const inputCell = document.createElement("td");
    inputCell.colSpan = 2;
    inputCell.appendChild(input);
    inputRow.appendChild(inputCell);

    const statsCell = document.createElement("td");
    statsCell.colSpan = 2;
    statsCell.id = `team-builder-${index}-${i}-stats`;
    statsCell.classList.add("team-builder-stats");
    statsRow.appendChild(statsCell);
  }
  const statsTd = document.createElement("td");
  statsTd.classList.add("team-builder-stats-cell");
  const statsNode = document.createElement("pre");
  statsTd.rowSpan = 3;
  statsTd.colSpan = 4;
  statsNode.id = `team-builder-${index}-stats`;
  statsNode.classList.add("team-builder-stats");
  statsTd.appendChild(statsNode);
  inputRow.appendChild(statsTd);

  const parkingTd = document.createElement("td");
  parkingTd.classList.add("team-builder-parking-cell");
  const parkingNode = document.createElement("pre");
  parkingTd.rowSpan = 3;
  parkingTd.colSpan = 6;
  parkingNode.id = `team-builder-${index}-parking`;
  parkingNode.classList.add("team-builder-parking");
  parkingTd.appendChild(parkingNode);
  inputRow.appendChild(parkingTd);

  const supportRow = document.createElement("tr");
  const supportHeader = CreateNode("th", document.createTextNode("Support"));
  supportHeader.rowSpan = 2;
  supportRow.appendChild(supportHeader);
  const supportStatsRow = document.createElement("tr");
  for (let i = 0; i < kMaxSupportTeamSize + 1; ++i) {
    const thumbCell = document.createElement("td");
    thumbCell.id = `team-builder-${index}-${i}-support-thumb`;
    thumbCell.classList.add("support-team-thumb");
    supportRow.appendChild(thumbCell);

    const statsCell = document.createElement("td");
    statsCell.id = `team-builder-${index}-${i}-support-stats`;
    statsCell.classList.add("team-builder-stats");
    supportStatsRow.appendChild(statsCell);
  }
  const tbody = document.createElement("tbody");
  tbody.appendChild(inputRow);
  tbody.appendChild(thumbRow);
  tbody.appendChild(statsRow);
  tbody.appendChild(supportRow);
  tbody.appendChild(supportStatsRow);
  const container = document.getElementById("team-builder");
  container.appendChild(CreateNode("table", tbody));
}

function RenderTeamImpl(teamIndex, context) {
  const statsNode = document.getElementById(`team-builder-${teamIndex}-stats`);
  statsNode.innerText =
    `Power:         ${context.power.toLocaleString().padStart(7)}\n` +
    `- Base:        ${context.powerDetailed[0].toLocaleString().padStart(7)}\n` +
    `- Area Item:   ${context.powerDetailed[1].toLocaleString().padStart(7)}\n` +
    `- Char Rank:   ${context.powerDetailed[2].toLocaleString().padStart(7)}\n` +
    `- Titles:      ${context.powerDetailed[3].toLocaleString().padStart(7)}\n` +
    `Skill Value:   ${context.skillValue.toLocaleString().padStart(6)}%\n` +
    `Event Bonus:   ${context.eventBonus.toLocaleString().padStart(6)}%\n` +
    `Est. EP:       ${context.expectedEp.toLocaleString().padStart(7)}\n`;
  if (context.supportBonus) {
    statsNode.innerText +=
      `\nEvent Bonus Breakdown\n` +
      `Main Bonus:    ${context.mainBonus.toLocaleString().padStart(6)}%\n` +
      `Support Bonus: ${context.supportBonus.toLocaleString().padStart(6)}%\n` +
      `Total Bonus:   ${context.eventBonus.toLocaleString().padStart(6)}%\n`;
  }
  const parkingNode = document.getElementById(`team-builder-${teamIndex}-parking`);
  parkingNode.innerText = "";
  if (context.expectedScore) {
    parkingNode.innerText +=
      `Challenge Live Details\n` +
      `Est. CL Score: ${context.expectedScore.toLocaleString().padStart(9)}\n` +
      `Best CL Song:  ${context.bestSongName}\n`;
  }
  if (context.parkingDetails) {
    if (parkingNode.innerText != "") {
      parkingNode.innerText += "\n";
    }
    parkingNode.innerText +=
      `Target EP: ${context.parkingDetails.target}\n`;
    if (context.parkingDetails.strategies) {
      parkingNode.innerText += "\nSingle-Turn Park Found (choose one)\n";
      parkingNode.innerText += "\nCan           Score Range    EP Mult";
      parkingNode.innerText += "\n------------------------------------\n";
      Array.from(context.parkingDetails.strategies).forEach((s) => {
        const boost = String(s.boost).padStart(2);
        const range = `${s.scoreLb.toLocaleString().padStart(9)} ~ ${s.scoreUb.toLocaleString().padStart(9)}`;
        const ep = s.baseEp.toLocaleString().padStart(5);
        const mult = `x${s.multiplier}`.padStart(4);
        parkingNode.innerText += `${boost}x ${range} ${ep} ${mult}\n`;
      });

      parkingNode.innerText += `\nMax Solo Ebi Score: ${context.parkingDetails.maxScore.toLocaleString()}`;
    } else if (context.parkingDetails.multiTurnStrategies) {
      parkingNode.innerText += "\nMulti-Turn Park Found (play all)\n";
      parkingNode.innerText += "\nCan           Score Range    EP Mult Plays";
      parkingNode.innerText += "\n------------------------------------------\n";
      Array.from(context.parkingDetails.multiTurnStrategies).forEach((s) => {
        const boost = String(s.boost).padStart(2);
        const range = `${s.scoreLb.toLocaleString().padStart(9)} ~ ${s.scoreUb.toLocaleString().padStart(9)}`;
        const ep = s.baseEp.toLocaleString().padStart(5);
        const mult = `x${s.multiplier}`.padStart(4);
        const plays = `x${s.plays.toLocaleString()}`.padStart(5);
        parkingNode.innerText += `${boost}x ${range} ${ep} ${mult} ${plays}\n`;
      });

      parkingNode.innerText += `\nMax Solo Ebi Score: ${context.parkingDetails.maxScore.toLocaleString()}`;
    } else {
      parkingNode.innerText += "\nNot viable or compute budget exhausted.";
    }
  }
  let offset = 0;
  for (let i = 0; i < 5; ++i) {
    const cardStatsNode = document.getElementById(`team-builder-${teamIndex}-${i}-stats`);
    cardStatsNode.innerText = "";
    const thumbNode = document.getElementById(`team-builder-${teamIndex}-${i}-thumb`);
    while (thumbNode.lastChild) {
      thumbNode.removeChild(thumbNode.lastChild);
    }
    const idInput = document.getElementById(`team-builder-${teamIndex}-${i}-id`);
    if (context.cards != undefined && (i + offset) < context.cards.length) {
      const card = context.cards[i + offset];
      if (card.cardId != idInput.value) {
        --offset;
        continue;
      }
      const bonusContrib = card.state.teamBonusContrib ?? 0;
      cardStatsNode.innerText =
        `Lv ${card.state.level}/MR ${card.state.masterRank}/SL ${card.state.skillLevel}\n` +
        `Power: ${card.state.teamPowerContrib}\n` +
        `Skill: ${card.state.teamSkillContrib}%\n` +
        `Bonus: ${bonusContrib}%\n`;
      thumbNode.appendChild(CreateCardThumb(card));
    }
  }
  for (let i = 0; i < kMaxSupportTeamSize; ++i) {
    const thumbCell = document.getElementById(
      `team-builder-${teamIndex}-${i}-support-thumb`);
    const statsCell = document.getElementById(
      `team-builder-${teamIndex}-${i}-support-stats`);
    while (thumbCell.lastChild) {
      thumbCell.removeChild(thumbCell.lastChild);
    }

    if (context.supportCards && i < context.supportCards.length) {
      const card = context.supportCards[i];
      thumbCell.appendChild(CreateCardThumbHover(card));
      statsCell.innerText =
        `MR${card.state.masterRank}/SL${card.state.skillLevel}\n` +
        `${card.state.teamBonusContrib}%\n`;
    } else {
      statsCell.innerText = "";
    }
  }
}

function ImportCsv() {
  let file = document.getElementById("upload-cards").files[0];
  let reader = new FileReader();
  reader.onload = (e) => {
    controller.ImportCardsFromCsv(e.target.result);
  }
  reader.readAsText(file);
}

function ImportBinaryProto() {
  let file = document.getElementById("upload-cards").files[0];
  let reader = new FileReader();
  reader.onload = (e) => {
    controller.ImportDataFromProto(e.target.result);
  }
  reader.readAsText(file);
}

function ImportTextProto() {
  let file = document.getElementById("upload-cards").files[0];
  let reader = new FileReader();
  reader.onload = (e) => {
    controller.ImportDataFromTextProto(e.target.result);
  }
  reader.readAsText(file);
}

function SaveAsFile(fileName, data, type) {
  var blob = new Blob([data], {type: type});
  var link = document.createElement('a');
  link.href = window.URL.createObjectURL(blob);
  link.download = fileName;
  link.click();
}

function ExportBinaryProto() {
  let data = controller.SerializeStateToString();
  SaveAsFile("profile.binaryproto", data, "application/octet-stream");
}

function ExportTextProto() {
  let data = controller.SerializeStateToDebugString();
  SaveAsFile("profile.textproto", data, "text/plain");
}

function BuildChallengeLiveTeam() {
  const selected = document.querySelector('input[name="cl-character-radio"]:checked');
  if (!selected) {
    document.getElementById("team-recommender-output").innerText =
      "No character selected for challenge live.";
  }
  controller.BuildChallengeLiveTeam(parseInt(selected.value));
}

function SetTargetPoints(e) {
  if (!e.validity.valid) {
    return;
  }

  const value = parseInt(e.value);
  if (value < 100) {
    e.setCustomValidity(
      "Invalid target point value (must be whole number at least 100)");
    return;
  }

  if (e.value == "") {
    controller.SetTargetPoints(0);
  } else {
    controller.SetTargetPoints(value);
  }
}

function SetParkAccuracy(e) {
  if (!e.validity.valid) {
    return;
  }

  const value = parseInt(e.value);
  if (value <= 0 || value > 100) {
    e.setCustomValidity(
      "Invalid target point value (must be whole number at least 0 and at most 100)");
    return;
  }

  if (e.value == "") {
    controller.SetParkAccuracy(0);
  } else {
    controller.SetParkAccuracy(value);
  }
}

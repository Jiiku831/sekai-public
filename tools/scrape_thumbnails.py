import json
import os

from absl import app
from absl import flags

FLAGS = flags.FLAGS

flags.DEFINE_string("cards", None, "input json")
flags.mark_flag_as_required("cards")

_PATH = "https://storage.sekai.best/sekai-jp-assets/thumbnail/chara/"


def main(argv):
    with open(FLAGS.cards) as f:
        cards = json.loads(f.read())
    assets = []
    for card in cards:
        if card["initialSpecialTrainingStatus"] != "done":
            assets.append(os.path.join(_PATH, card["assetbundleName"] + "_normal.webp"))
        if card["cardRarityType"] == "rarity_3" or card["cardRarityType"] == "rarity_4":
            assets.append(
                os.path.join(_PATH, card["assetbundleName"] + "_after_training.webp")
            )

    print("\n".join(assets))


if __name__ == "__main__":
    app.run(main)

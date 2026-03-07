import os
import sys
import pathlib

from google.protobuf import json_format
from sekai.run_analysis import bridge
from sekai.run_analysis.proto import service_pb2


def main():
    bridge.init_module()
    data_dir = pathlib.Path(sys.argv[1])
    request = service_pb2.AnalyzePlayerRequest()
    request.compute_stats = True
    for file in os.listdir(data_dir):
        with open(data_dir / file) as f:
            request.MergeFrom(
                json_format.Parse(f.read(), service_pb2.AnalyzePlayerRequest())
            )
    response = service_pb2.AnalyzePlayerResponse()
    response.ParseFromString(
        bridge.send_request("/analyze_player", request.SerializeToString())
    )

    with open(sys.argv[2], "w") as f:
        f.write(f"result = {json_format.MessageToJson(response)};")


if __name__ == "__main__":
    main()

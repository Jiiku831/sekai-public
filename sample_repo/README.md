## Saving your profile in a private repo

1. Copy the contents of this directory into your repo.
2. Clone the `sekai-public` repo somewhere and update `MODULE.bazel` to point to that repo. You will
   need to pull this repo every time there is an update to the program.
3. Update the `sekai-master-db` version in the `MODULE.bazel` file to match the version in
   `sekai-public/MODULE.bazel`. You will need to do this every time there is a data update.

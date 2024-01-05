set(SPDLOG_FMT_EXTERNAL ON)
cpmaddpackage(
        NAME spdlog
        GIT_REPOSITORY "git@github.com:gabime/spdlog.git"
        GIT_TAG v1.12.0)
unset(SPDLOG_FMT_EXTERNAL)

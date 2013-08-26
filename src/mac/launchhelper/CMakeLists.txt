cmake_minimum_required(VERSION 2.8)

project (zulip-launchhelper)

# This is the OS X launcher app that used to enable the start-on-login
# functionality. It is installed as an app bundle  **inside** the Zulip.app
# bundle

add_subdirectory(src)

#pragma once

#include "test/rely.h"
#include "util/wins_util/clipboard.hpp"

namespace test
{
    void test()
    {
        // wins_util::Clipboard::setText("sljtcs");
        std::string text = wins_util::Clipboard::getText();
        LOG("clipboard: ", text);
    }
}
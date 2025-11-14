#pragma once

#ifndef LOCALE
#define LOCALE EN
#endif

#if (LOCALE == EN)
#include "help/english.hpp"
#else
#error "Unknown LOCALE specified"
#endif

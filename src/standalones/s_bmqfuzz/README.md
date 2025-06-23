# Fuzzing set up

This folder contains fuzzers for blazingmq

- `s_bmqfuzz_eval.cpp` targets `BloombergLP::bmqeval::SimpleEvaluator::compile`
- `s_bmqfuzz_parseutil.cpp` targets `BloombergLP::mqbcmd::ParseUtil::parse`
- `s_bmqfuzz_bmqt_uri.cpp` targets `BloombergLP::bmqt::UriParser::parse`

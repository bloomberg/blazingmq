# Fuzzing set up

This folder contains fuzzers for blazingmq

- `eval_fuzzer.cpp` targets `BloombergLP::bmqeval::SimpleEvaluator::compile`
- `parseutil_fuzzer.cpp` targets `BloombergLP::mqbcmd::ParseUtil::parse`
- `bmqt_uri_fuzzer.cpp` targets `BloombergLP::bmqt::UriParser::parse`


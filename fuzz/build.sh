cd $SRC/zip

mkdir -p build
cmake -S . -B build -DCMAKE_C_COMPILER_WORKS=1 -DZIP_BUILD_FUZZ=ON && cmake --build build --target install

# Prepare corpora
zip -q $OUT/read_entry_fuzzer_seed_corpus.zip fuzz/corpus/*
cp $OUT/read_entry_fuzzer_seed_corpus.zip $OUT/create_zip_fuzzer_seed_corpus.zip

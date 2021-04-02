add_library(intDataStructures bIntSet.cpp bIntSet.h
        bucketSet.cpp bucketSet.h
        delIterIntSet.cpp delIterIntSet.h
        FlexIntStack.cpp FlexIntStack.h
        IntPairHeap.h
        IntStack.cpp IntStack.h
        IntUtil.cpp IntUtil.h
        noDelIntSet.cpp noDelIntSet.h
        StringUtil.cpp StringUtil.h
		HashTable.cpp HashTable.h
		SequenceSet.cpp SequenceSet.h
		CompressedSequenceSet.cpp CompressedSequenceSet.h
		SequenceSetCommon.h
		)

target_compile_options(intDataStructures PUBLIC ${compiler_flags})
target_link_options(intDataStructures PUBLIC ${linker_flags})


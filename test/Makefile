RUN_TESTS = $(shell echo *.test)
SH_TESTS = $(shell echo *.shtest)

TESTS = ${RUN_TESTS} ${SH_TESTS}

RESULTS = ${TESTS:test=test_result}

all: ${RESULTS}

%shtest_result: %shtest
	echo SH test $<

%.test_result: %.test
	echo RUN test $<

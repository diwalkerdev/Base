extern void
test_smallmath_main();

extern void
Test_Array();

extern void
test_backfill_vector_main();

extern void
Test_DebugServices();

extern void
Test_VirtualMemory();

extern void
Test_RelativePointers();

int
main()
{
    test_smallmath_main();
    Test_Array();
    test_backfill_vector_main();
    Test_VirtualMemory();
    Test_DebugServices();
    Test_RelativePointers();
}

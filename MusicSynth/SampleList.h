//--------------------------------------------------------------------------------------------------
// SampleList.h
//
// The list of samples loaded from the Samples Directory.
// Used to generate boilerplate code via macro expansion.
// Creates g_sample_<name> variables for each sample loaded.
// Loads file "Samples/<name>.wav"
//
//--------------------------------------------------------------------------------------------------

SAMPLE(clap)
SAMPLE(cymbal)
SAMPLE(kick)
SAMPLE(legend1)
SAMPLE(legend2)
SAMPLE(ting)
//SAMPLE(oakenfold)
SAMPLE(pvd)

#undef SAMPLE
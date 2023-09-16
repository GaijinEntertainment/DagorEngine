#pragma once

#define DAS_PROFILE_SECTIONS 0

extern "C" int64_t ref_time_ticks ();
extern "C" int get_time_usec ( int64_t reft );
extern "C" int64_t get_time_nsec ( int64_t reft );

#if DAS_PROFILE_SECTIONS

#define DAS_PROFILE_SECTION(name) das::ProfileSection section_##__LINE__ (name);

namespace das {
    class ProfileSection {
    public:
        ProfileSection(const char * n = nullptr);
        ~ProfileSection();
    private:
        const char * name = nullptr;
        uint64_t timeStamp = 0;
    };
}

#else

#define DAS_PROFILE_SECTION(name)

#endif

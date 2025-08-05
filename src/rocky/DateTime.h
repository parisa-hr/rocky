/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky/Common.h>
#include <ctime>
#include <string>

namespace ROCKY_NAMESPACE
{
    /** Basic timestamp (seconds from the 1970 epoch) */
    using TimeStamp = std::int64_t;

    /** Time span (in seconds) */
    using TimeSpan = long;

    /**
     * General-purpose UTC date/time object.
     * One second resolution, GMT time zone.
     */
    class ROCKY_EXPORT DateTime
    {
    public:
        /** DateTime representing "now" */
        DateTime();

        /** DateTime copy */
        DateTime(const DateTime& rhs);

        /** DateTime from a tm (in the local time zone) */
        DateTime(const ::tm& tm);

        /** DateTime from UTC seconds since the epoch */
        DateTime(TimeStamp utc);

        /** DateTime from year, month [1-12], date [1-31], hours [0-24) */
        DateTime(int year, int month, int day, double hours);

        /** DateTime from year and fractional day-of-year [1..365] */
        DateTime(int year, double dayOfYear);

        /** DateTime from an ISO 8601 string */
        DateTime(const std::string& iso8601);

        /** As a date/time string in RFC 1123 format (e.g., HTTP) */
        const std::string asRFC1123() const;

        /** As a date/time string in ISO 8601 format (lexigraphic order). */
        const std::string asISO8601() const;

        /** As a date/time string in compact ISO 8601 format (lexigraphic
          * order with no delimiters). */
        const std::string asCompactISO8601() const;

        /** Julian day (fractional) corresponding to this DateTime */
        double getJulianDay() const;

        /** Seconds since Jan 1, 1970 00:00 UTC */
        TimeStamp asTimeStamp() const { return _time_t; }

        /** Adds hours to return a new DateTime */
        DateTime operator + (double hours) const;

        //! Compare functions
        bool operator < (const DateTime& rhs) const {
            return asTimeStamp() < rhs.asTimeStamp();
        }

        bool operator > (const DateTime& rhs) const {
            return asTimeStamp() > rhs.asTimeStamp();
        }

        bool operator == (const DateTime& rhs) const {
            return asTimeStamp() == rhs.asTimeStamp();
        }

    public:
        int    year()  const;
        int    month() const;
        int    day()   const;
        double hours() const;

    protected:
        ::tm     _tm;
        ::time_t _time_t;

    private:
        // since timegm is not cross-platform
        ::time_t timegm(const ::tm* tm) const;
        void parse(const std::string& in);
    };


    class ROCKY_EXPORT DateTimeExtent
    {
        public:
            DateTimeExtent() : _valid(false)
            {
                //nop
            }

            DateTimeExtent(const DateTime& start, const DateTime& end) :
                _start(start), _end(end), _valid(true)
            {
                //nop
            }

            const bool valid() const {
                return _valid;
            }

            const DateTime& getStart() const {
                return _start;
            }

            const DateTime& getEnd() const {
                return _end;
            }

            void expandBy(const DateTime& value);

        private:
            bool _valid;
            DateTime _start;
            DateTime _end;
    };

} // namespace ROCKY_NAMESPACE


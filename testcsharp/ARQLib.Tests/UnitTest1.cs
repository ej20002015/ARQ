using System;
using Xunit;

namespace ARQ.Tests
{
    public class UuidTests
    {
        [Fact]
        public void Test_Uuid_Bidirectional_Conversion()
        {
            // ---------------------------------------------------------
            // Part 1: C++ -> C#
            // ---------------------------------------------------------
            var cppUuid = ARQ.ID.UUID.create();
            Guid csharpGuid = cppUuid; // Trigger implicit cast
            
            Assert.NotEqual(Guid.Empty, csharpGuid);
            Console.WriteLine($"Generated from C++: {csharpGuid}");

            // ---------------------------------------------------------
            // Part 2: C# -> C++ -> C# (The Round-Trip)
            // ---------------------------------------------------------
            // Use a specific known Guid to verify the bytes don't get scrambled
            Guid originalCSharpGuid = Guid.Parse("018b1234-abcd-7890-ef12-3456789abcde");

            // Act: Trigger implicit cast FROM C# TO C++
            ARQ.ID.UUID convertedToCpp = originalCSharpGuid;

            // Act: Trigger implicit cast FROM C++ BACK TO C#
            Guid roundTrippedGuid = convertedToCpp;

            // Assert: The original and the round-tripped Guid must be perfectly identical
            Assert.Equal(originalCSharpGuid, roundTrippedGuid);

            // Assert: The ToString() method should render the exact same format
            Assert.Equal("018b1234-abcd-7890-ef12-3456789abcde", convertedToCpp.ToString());

            Console.WriteLine($"Successfully round-tripped C# Guid: {roundTrippedGuid}");
        }
    }

    public class TimeTests
    {
        [Fact]
        public void Test_Date_Bidirectional_Conversion()
        {
            // ---------------------------------------------------------
            // Part 1: Populated Date Round-Trip
            // ---------------------------------------------------------
            DateOnly originalDate = new DateOnly(2026, 5, 22);

            // Act: C# -> C++
            ARQ.Time.Date cppDate = originalDate;

            // Act: C++ -> C#
            DateOnly? roundTrippedDate = cppDate;

            // Assert
            Assert.True(roundTrippedDate.HasValue);
            Assert.Equal(originalDate, roundTrippedDate.Value);
            Console.WriteLine($"Successfully round-tripped Date: {roundTrippedDate.Value:O}");

            // ---------------------------------------------------------
            // Part 2: Unpopulated (Null) Date Round-Trip
            // ---------------------------------------------------------
            DateOnly? nullDate = null;
            
            // Should hit the `ymd == 0` sentinel and create an empty C++ Date
            ARQ.Time.Date cppNullDate = nullDate; 
            DateOnly? roundTrippedNull = cppNullDate;

            Assert.False(roundTrippedNull.HasValue);
        }

        [Fact]
        public void Test_DateTime_Bidirectional_Conversion()
        {
            // ---------------------------------------------------------
            // Part 1: Populated DateTime Round-Trip (Microsecond Precision)
            // ---------------------------------------------------------
            // C# Ticks are 100ns. 1 microsecond = 10 ticks.
            // Let's create a UTC time with exactly 123,456 microseconds past the second.
            long baseTicks = new DateTime(2026, 5, 22, 14, 30, 15, DateTimeKind.Utc).Ticks;
            long microsecondTicks = 123456 * 10; 
            
            DateTime originalTime = new DateTime(baseTicks + microsecondTicks, DateTimeKind.Utc);

            // Act: C# -> C++
            ARQ.Time.DateTime cppTime = originalTime;

            // Act: C++ -> C#
            DateTime? roundTrippedTime = cppTime;

            // Assert
            Assert.True(roundTrippedTime.HasValue);
            Assert.Equal(originalTime, roundTrippedTime.Value);
            
            // Format "O" prints down to the 7th decimal place, proving no precision was lost
            Assert.Equal("2026-05-22T14:30:15.1234560Z", roundTrippedTime.Value.ToString("O"));
            Console.WriteLine($"Successfully round-tripped DateTime: {roundTrippedTime.Value:O}");

            // ---------------------------------------------------------
            // Part 2: Unpopulated (Null) DateTime Round-Trip
            // ---------------------------------------------------------
            DateTime? nullTime = null;
            
            // Should hit the `long.MinValue` sentinel and create an empty C++ DateTime
            ARQ.Time.DateTime cppNullTime = nullTime;
            DateTime? roundTrippedNull = cppNullTime;

            Assert.False(roundTrippedNull.HasValue);
        }
    }
}
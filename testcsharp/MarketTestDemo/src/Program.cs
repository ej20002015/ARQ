using TMQ = TMQLib.TMQ;

TMQ.Time.Date date = new TMQ.Time.Date();
System.Console.WriteLine(date.isSet());
System.Console.WriteLine($"{date.year().toInt()}-{date.month().ToString()}-{date.day().toInt()}");

try
{
    TMQ.Time.Date date2 = new TMQ.Time.Date(new TMQ.Time.Year(2025), TMQ.Time.Month.May, new TMQ.Time.Day(40));
    System.Console.WriteLine(date2.isSet());
    System.Console.WriteLine($"{date2.year().toInt()}-{date2.month().ToString()}-{date2.day().toInt()}");
}
catch (Exception e)
{
    Console.WriteLine(e.ToString());
}

TMQ.Time.Date date3 = new TMQ.Time.Date(TMQ.Time.TimeZone.Local);
System.Console.WriteLine(date3.isSet());
System.Console.WriteLine($"{date3.year().toInt()}-{date3.month().ToString()}-{date3.day().toInt()}");

TMQ.Time.Date date4 = new TMQ.Time.Date(0);
System.Console.WriteLine(date4.isSet());
System.Console.WriteLine($"{date4.year().toInt()}-{date4.month().ToString()}-{date4.day().toInt()}");

TMQLib.TMQ.Mkt.Context context = new TMQLib.TMQ.Mkt.Context("EOD", date3);
Console.WriteLine(context.str());

var context2 = TMQLib.TMQ.Mkt.Context.LIVE;
Console.WriteLine(context2.str());

var context3 = new TMQLib.TMQ.Mkt.Context("LIVE_EVAN");
Console.WriteLine(context3.str());

//// See https://aka.ms/new-console-template for more information
//using MarketTestDemo.src.TMQ.Extensions;
//using System.Runtime.CompilerServices;
//using System.Runtime.InteropServices;
//using TMQTime = TMQ.TMQUtils.Time;

//Console.WriteLine("Hello, World!");

//int result = market_test.add(2, 3);
//Console.WriteLine($"Result: {result}");

////Foo? val = market_test.getFoo(false);
////if (val != null)
////    throw new Exception();

////using(Foo val3 = market_test.fooVal())
////{
////    Console.WriteLine($"{val3.ToString()}, add = {val3.add2()}"); // Should be non-null
////}

////using (Foo? val2 = market_test.getFoo(true))
////{
////    if (val2 == null)
////        throw new Exception();

////    Console.WriteLine($"{val2?.ToString()}, add = {val2?.add2()}"); // Should be non-null
////}

////Foo? val2 = market_test.getFoo(true);
////if (val2 == null)
////    throw new Exception();

////FooWeakPtr weak1 = market_test.fooWeakPtr();

////Foo foo = weak1.lock_();
////if (foo != null)
////    Console.WriteLine(foo.ToString());
////else
////    Console.WriteLine("foo is null"); // Should be null

////market_test.makeFoo();

////weak1 = market_test.fooWeakPtr();
////Console.WriteLine(weak1.use_count()); // Should be 1

////foo = weak1.lock_();
////if (foo != null)
////    Console.WriteLine($"{foo.ToString()}, add = {foo.add2()}"); // Should be non-null
////else
////    Console.WriteLine("foo is null");

////var fooShared = market_test.fooSharedPtr();
////Console.WriteLine($"{fooShared.ToString()}, add = {fooShared.add2()}"); // What will val be?

////Console.WriteLine(weak1.use_count()); // Should be 3

////market_test.deleteFoo();
////Console.WriteLine(weak1.use_count()); // Should be 2

////foo = weak1.lock_();
////if (foo != null)
////    Console.WriteLine($"{foo.ToString()}, add = {foo.add2()}"); // Should be non-null
////else
////    Console.WriteLine("foo is null");

////fooShared = null;
////foo = null;

////System.GC.Collect();

////Console.WriteLine(weak1.use_count()); // Should be 0?

////foo = weak1.lock_();
////foo = weak1.lock_();
////if (foo != null)
////    Console.WriteLine($"{foo.ToString()}, add = {foo.add2()}");
////else
////    Console.WriteLine("foo is null"); // Should be null

////weak1 = market_test.fooWeakPtr();
////Console.WriteLine(weak1.use_count()); // Should be 0

////foo = weak1.lock_();
////if (foo != null)
////    Console.WriteLine($"{foo.ToString()}, add = {foo.add2()}");
////else
////    Console.WriteLine("foo is null"); // Should be null

////using (Foo? g_foo = market_test.getGlobalFoo(false))
////{
////    if (g_foo != null)
////        throw new Exception();
////}

////using (Foo? g_foo2 = market_test.getGlobalFoo(true))
////{
////    if (g_foo2 == null)
////        throw new Exception();
////    Console.WriteLine($"Add = {g_foo2.add2()}"); // Should be 10
////    market_test.changeGlobalFoo();
////    Console.WriteLine($"Add = {g_foo2.add2()}"); // Should be 10
////}

////using (Foo? g_foo2 = market_test.getGlobalFoo(false))
////{
////    if (g_foo2 == null)
////        throw new Exception();
////    Console.WriteLine($"Add = {g_foo2.add2()}"); // Should be 20
////    market_test.deleteGlobalFoo();
////    // Console.WriteLine($"Add = {g_foo2.add2()}"); // This call would access a deleted C++ object, causing AccessViolationException
////}

////Foo? g_foo3 = market_test.getGlobalFoo(false);
////if (g_foo3 != null)
////    throw new Exception();


//TMQTime.Date date = new TMQTime.Date();
//System.Console.WriteLine(date.isSet());
//System.Console.WriteLine($"{date.year().toInt()}-{date.month().ToString()}-{date.day().toInt()}");

//TMQTime.Date date2 = new TMQTime.Date(new TMQTime.Year(2025), TMQTime.Month.May, new TMQTime.Day(40));
//System.Console.WriteLine(date2.isSet());
//System.Console.WriteLine($"{date2.year().toInt()}-{date2.month().ToString()}-{date2.day().toInt()}");

//TMQTime.Date date3 = new TMQTime.Date(TMQTime.TimeZone.Local);
//System.Console.WriteLine(date3.isSet());
//System.Console.WriteLine($"{date3.year().toInt()}-{date3.month().ToString()}-{date3.day().toInt()}");

//TMQTime.Date date4 = new TMQTime.Date(0);
//System.Console.WriteLine(date4.isSet());
//System.Console.WriteLine($"{date4.year().toInt()}-{date4.month().ToString()}-{date4.day().toInt()}");

//var dateTest = new TMQLib.TMQUtils.Time.Date(0);
//var only = dateTest.ToDateOnly();
//System.Console.WriteLine(only);

//var onlyDate = only.ToTMQDate();
//var onlyBack = onlyDate.ToDateOnly();
//System.Console.WriteLine(onlyBack);



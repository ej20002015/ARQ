#pragma once

namespace ARQ
{

using ARQDynaLibInitFunc     = void( * )( ); // Func name needs to be arqDynaLibInit
using ARQDynaLibShutdownFunc = void( * )( ); // Func name needs to be arqDynaLibShutdown

}
#include <cstdio>
#include <cstring>

#include "pio.hxx"
#include "tio.hxx"
#include "memory.hxx"
#include "symtab.hxx"
#include "error.hxx"

SEXPR PIO::stdin_port;
SEXPR PIO::stdout_port;
SEXPR PIO::stderr_port;
SEXPR PIO::terminal_port;
SEXPR PIO::eof_object;

FILE* PIO::transcript = 0;


SEXPR PIO::open( const char* name, short mode, const char* ftype )
{
   FILE* file = fopen(name, ftype);
  
   if (file == NULL)
   {
      return null;
   }
   else
   {
      SEXPR port = MEMORY::port(0, mode);
      setfile(port, file);
      return port;
   }
}

SEXPR PIO::open_on_string( SEXPR str, short mode )
{
   SEXPR port = MEMORY::string_port();
   setmode(port, mode);
   setstringportstring(port, str);
   return port;
}

void PIO::set_position( SEXPR port, SEXPR pos )
{
   if ( port == terminal_port )
   {
      ERROR::warning( "set_position on terminal port", port );
      return;
   }

   if ( getfile(port) == NULL )
      ERROR::severe( "set_position on closed port", port );

   if (-1 == fseek(getfile(port), getfixnum(pos), SEEK_SET))
      ERROR::severe("seek error on file");
}

SEXPR PIO::get_position( SEXPR port )
{
   if ( port == terminal_port )
   {
      ERROR::warning( "get_position on terminal port", port );
      return MEMORY::fixnum(0);
   }

   if ( getfile(port) == NULL )
      ERROR::severe( "get_position on closed port", port );

   return MEMORY::fixnum(static_cast<FIXNUM>(ftell(getfile(port))));
}

void PIO::close( SEXPR port )
{
   if ( port == terminal_port )
   {
      ERROR::warning( "close on terminal port", port );
      return;
   }

   if ( getfile(port) == NULL )
      ERROR::severe( "close on closed port", port );

   fclose(getfile(port));
   setfile(port, NULL);
}

void PIO::remove( const char* name )
{
   ::remove(name);
}

void PIO::flush( SEXPR port )
{
   if ( port == terminal_port )
   {
      ERROR::warning( "flush on terminal port", port );
      return;
   }

   if ( getfile(port) == NULL )
      ERROR::severe( "flush on closed port", port );

   fflush( getfile(port) );
}

//
// Dual Use (standard ports and string ports)
//

int PIO::get( SEXPR inport )
{
   if ( inportp(inport) )
   {
      if ( getfile(inport) == NULL )
	 ERROR::severe( "get on closed port", inport );

      if ( inport == terminal_port )
	 return TIO::terminal_getch();
      else if ( inport == stdin_port )
	 return TIO::getch();
      else
	 return fgetc( getfile(inport) );
   }
   else if ( instringportp(inport) )
   {
      // string input
      SEXPR s = getstringportstring(inport);
      const int ch = getstringdata(s)[getstringindex(s)];
      if (ch == 0)
      {
	 return EOF;
      }
      else
      {
	 getstringindex(s)++;
	 return ch;
      }
   }
   else
   {
      ERROR::severe( "not an input port", inport );
      return 0;
   }
}

void PIO::unget( SEXPR inport, int ch )
{
   if ( inportp(inport) )
   {
      if ( getfile(inport) == NULL )
	 ERROR::severe( "unget on closed port", inport );

      if ( inport == terminal_port )
	 return TIO::terminal_unget( ch );
      else if ( inport == stdin_port )
	 TIO::unget( ch );
      else
	 ungetc( ch, getfile(inport) );
   }
   else if ( instringportp(inport) )
   {
      if (ch != EOF)
      {
	 // string input (adjust)
	 SEXPR s = getstringportstring(inport);
	 if (getstringindex(s) > 0)
	    getstringindex(s)--;
      }
   }
   else
   {
      ERROR::severe( "not an input port", inport );
   }
}

static void append_char( SEXPR str, int ch )
{
   // append the character, expanding if needed
   if (getstringindex(str) >= getstringlength(str))
   {
      str = MEMORY::string_resize( str, 1000 );
   }
   getstringdata(str)[getstringindex(str)++] = ch;
   getstringdata(str)[getstringindex(str)] = 0;
}

void PIO::put( SEXPR outport, const char* s )
{
   if ( outportp(outport) )
   {
      if ( getfile(outport) == NULL )
	 ERROR::severe( "put on closed port", outport );

      if ( transcript && (outport == stdout_port) )
	 fputs( s, transcript );

      fputs( s, getfile(outport) );
   }
   else if ( outstringportp(outport) )
   {
      // string output
      SEXPR str = getstringportstring(outport);
      while (*s)
	 append_char(str, *s++);
   }
   else
   {
      ERROR::severe( "not an output port", outport );
   }
}

void PIO::put( SEXPR outport, int ch )
{
   if ( outportp(outport) )
   {
      if ( getfile(outport) == NULL )
	 ERROR::severe( "put on closed port", outport );

      if ( transcript && (outport == stdout_port) )
	 fputc( ch, transcript );

      fputc( ch, getfile(outport) );
   }
   else if ( outstringportp(outport) )
   {
      append_char( getstringportstring(outport), ch );
   }
   else
   {
      ERROR::severe( "not an output port", outport );
   }
}

void PIO::transcript_on( const char* name )
{
   if ( transcript )
      fclose( transcript );

   transcript = fopen( name, "w" );

   if ( transcript == NULL )
      ERROR::severe( "unable to open transcript file", MEMORY::string(name) );
}

void PIO::transcript_off()
{
   if ( transcript )
      fclose( transcript );
}

static void pio_marker()
{
   // mark the PIO objects
   MEMORY::mark(PIO::stdin_port);
   MEMORY::mark(PIO::stdout_port);
   MEMORY::mark(PIO::stderr_port);
   MEMORY::mark(PIO::terminal_port);
   MEMORY::mark(PIO::eof_object);
}

void PIO::initialize()
{
   stdin_port = MEMORY::port(stdin, pm_input);
   SYMTAB::enter("*stdin*", stdin_port);
   SYMTAB::enter("*standard-input*", stdin_port);

   stdout_port = MEMORY::port(stdout, pm_output);
   SYMTAB::enter("*stdout*", stdout_port);
   SYMTAB::enter("*standard-output*", stdout_port);

   stderr_port = MEMORY::port(stderr, pm_output);
   SYMTAB::enter("*stderr*", stderr_port);
   SYMTAB::enter("*standard-error*", stderr_port);

   terminal_port = MEMORY::port(stdin, pm_input);
   SYMTAB::enter("*terminal*", terminal_port);

   TIO::history_init();

   eof_object = SYMTAB::enter("**eof**");
   setvalue(eof_object, eof_object);

   MEMORY::register_marker( pio_marker );
}
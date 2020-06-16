# INTRODUCTION TO CGI

Geomyidae has  support for running  scripts on each request,  which will
generate dynamic content.

There are two modes: standard cgi  and dynamic cgi. (»CGI« as name was
just taken, because that's easier to compare to the web.)


## PERMISSIONS

The scripts are run using the permissions of geomyidae. It is advised to
use the -g and -u options of geomyidae.


## BEFOREHAND

In these examples C: is what the  client sends and S: what the server is
sending.


## CALLING CONVENTION

Geomyidae will call the script like this:

	% $gopherroot/test.cgi $search $arguments $host $port

When it is a plain request, the arguments will have these values:

	C: /test.cgi
	-> $search = ""
	-> $arguments = ""
	-> $host = server host
	-> $port = server port

If the request is for a type 7 search element, then the entered string by
the user will be seen as following:

	C: /test.cgi	searchterm		(There is a TAB in-between)
	-> $search = »searchterm«
	-> $arguments = ""
	-> $host = server host
	-> $port = server port

When you are trying to give your script some calling arguments, the syntax
is:

	C: /test.cgi?hello
	-> $search = ""
	-> $arguments = »hello«
	-> $host = server host
	-> $port = server port

If both ways of input are combined, the variables are set as following:

	C: /test.cgi?hello=world	searchterm	(Beware! A Tab!)
	-> $search = »searchterm«
	-> $arguments = »hello=world«
	-> $host = server host
	-> $port = server port


## STANDARD CGI

The file  extension »cgi« switches to  this mode, where the  output of
the script is not interpreted at all  by the server and the script needs
to send raw content.

	% cat test.cgi
	#!/bin/sh
	echo "Hello my friend."
	%

The client will receive:

	S: Hello my friend.


## DYNAMIC CGI

For using  dynamic CGI, the  file needs to  end in »dcgi«,  which will
switch on  the interpretation of the  returned lines by the  server. The
interpreted for- mat is the same as in the .gph files.

	% cat test.dcgi
	#!/bin/sh
	echo "[1|Some link|/somewhere|server|port]"
	%

Here  geomyidae will  interpret the  .gph format  and return  the valid
gopher menu item.

	S: 1Some link	/somewhere	gopher.r-36.net	70

For outputting  large texts and  having minor hassles with  the content,
prepend »t« to every line beginning with »t« or all lines:

	% cat filereader.dcgi
	#!/bin/sh
	cat bigfile.txt | sed 's,^t,&&,'

## ENVIRONMENT VARIABLES

Please see the manpage geomyidae(8) for all variables and their content.


Have fun!


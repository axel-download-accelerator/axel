## HOW TO CONTRIBUTE TO AXEL DEVELOPMENT

Axel is available at https://github.com/axel-download-accelerator/axel

If you are interested in contribute to axel development, please, follow
these steps:

1. Send a patch that fix an issue or that implement a new feature.
   Alternatively, you can do a 'pull request'[1] in GitHub.

[1]: https://help.github.com/articles/using-pull-requests

2. Ask for join to the Axel project in GitHub, if you want to work
   officially. Note that this second step is not compulsory. However,
   to accept you in project, is needed a minimum previous collaboration.


To find issues and bugs to fix, you can check these addresses:

   - https://github.com/axel-download-accelerator/axel/issues
   - https://bugs.debian.org/cgi-bin/pkgreport.cgi?dist=unstable;package=axel
   - https://bugs.launchpad.net/ubuntu/+source/axel/+bugs
   - https://apps.fedoraproject.org/packages/axel/bugs
   - https://bugs.archlinux.org/?project=5&cat[]=33&string=axel
   - https://bugs.gentoo.org/buglist.cgi?quicksearch=net-misc%2Faxel

If you want to join, please make a contact.

There is a group here[2] to discuss and to coordinate the development.
You can also find other developers in the #axel channel on freenode.

[2]: https://groups.google.com/forum/#!forum/axel-accelerator-dev

  -- Eriberto, Sun, 20 Mar 2016 16:27:53 -0300,
     updated on Sun, 08 Sep 2017 23:27:00 -0300.

## Submitting Changes

### Coding style
As of version 2.15, Axel adopted a new coding style, very similar to that of the
Linux Kernel, with the additional requirement to insert a newline after the
return type of procedure declarations.

To aid the transition for imported code, an `.indent.pro` file is provided in
the top level source directory.  It should work with both GNU and BSD
implementations of `indent`, although the results may be slightly different.

Small variations are acceptable, and *non-compliance* in existing code, by
itself, *is not something to fix*.

### Licensing Rules
Axel is provided under the terms of the GNU General Public License version 2 or
(at your option) any later version, as described in the COPYING file, plus an
exception for linking against OpenSSL 1.x.

By submitting code for inclusion in the project you agree to license it under
these terms, or more permissive ones.

Here's the wording in the header of each file licensed under GPL-2.0:

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	In addition, as a special exception, the copyright holders give
	permission to link the code of portions of this program with the
	OpenSSL library under certain conditions as described in each
	individual source file, and distribute linked combinations including
	the two.

	You must obey the GNU General Public License in all respects for all
	of the code used other than OpenSSL. If you modify file(s) with this
	exception, you may extend this exception to your version of the
	file(s), but you are not obligated to do so. If you do not wish to do
	so, delete this exception statement from your version. If you delete
	this exception statement from all source files in the program, then
	also delete it here.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

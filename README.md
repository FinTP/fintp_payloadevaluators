fintp_payloadevaluators
=======================

Adapters for message formats

Requirements
------------
- Xerces-C
- Xalan-C
- **fintp_utils**
- **fintp_log**
- **fintp_base**

Build instructions
------------------
- On Unix-like systems, **fintp_payloadevaluators** uses the GNU Build System (Autotools) so we first need to generate the configuration script using:


        autoreconf -fi
Now we must run:

        ./configure
        make

- For Windows, a Visual Studio 2010 solution is provided.

License
-------
- [GPLv3](http://www.gnu.org/licenses/gpl-3.0.html)


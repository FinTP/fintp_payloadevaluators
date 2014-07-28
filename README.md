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
- **fintp_routingengine**

Build instructions
------------------
- On Unix-like systems, **fintp_payloadevaluators** uses the GNU Build System (Autotools) so we first need to generate the configuration script using:


        autoreconf -fi
Now we must run:

        ./configure
        make

- For Windows, a Visual Studio 2010 solution is provided.

Installation
------------
FinTP payload evaluators are plugins used by Routing Engine to add new functionality.
Routing Engine searches for plugins using the "PluginLocation" key in the appSettings section of RoutingEngine.config.

Example:

`<add key="PluginLocation" value="/home/fintp/fintp_payloadevaluators/.libs"/>`

For more information on how to configure Routing Engine, please consult our [fintp_samples repository](https://github.com/FinTP/fintp_samples).

License
-------
- [GPLv3](http://www.gnu.org/licenses/gpl-3.0.html)

Copyright
-------
COPYRIGHT.  Copyright to the SOFTWARE is owned by ALLEVO and is protected by the copyright laws of all countries and through international treaty provisions. 
NO TRADEMARKS.  Customer agrees to remove all Allevo Trademarks from the SOFTWARE (i) before it propagates or conveys the SOFTWARE or makes it otherwise available to third parties and (ii) as soon as the SOFTWARE has been changed in any respect whatsoever. 

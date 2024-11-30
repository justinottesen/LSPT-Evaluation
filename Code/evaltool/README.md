# Evaltool

Coming soon...

Copy pasted from main README:

> It may be worth implementing a feature in `evaltool` which simply receives and prints the received message (and maybe whether it is in compliance with our expectations).
>
> For example, command is `./evaltool.py proxy -p 9999 -f 8888` and it runs as a "proxy" evaluation component on port 9999 that just prints what it receives (and maybe forwards the message to 8888 where the actual evaluation component is running).
>
> In my experience, networking components together is one of the most annoying parts of a project like this, especially when everything is in different languages. This could be a very low effort way to significantly help with testing and debugging, and conclusively say who is the one messing up the interface.
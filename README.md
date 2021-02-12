# mc-autojoin

Normally, applications implementing multicast protocols need to join the
respective multicast group explicitly using the `IP_ADD_MEMBERSHIP`
sokcet option. However, such a join is bound to a specific interface
(either determined by the `ifindex`, the local IP given or a
routing-table-based decision (on Linux) depending on the multicast group
address if `INADDR_ANY` is given).

Thus, if interfaces appear/disappear dynamically, an application that
wants to handle a given multicast protocol on all interfaces must not
only iterate all of them during startup but also continuously track if
new interfaces have appeared. Not all applications are written with this
amount of dynamicism in mind. The Linux kernel can automatically join
multicast groups on a given interface using the `autojoin` mechanism
(see [this commit](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=93a714d6b53d87872e552dbb273544bdeaaf6e12)
for details), but this might not be available or desirable on a given
system.

`mc-autojoin` is a small utility that watches the set of available
network interfaces and joins the supplied IPv4 multicast group(s) on all
of them, relieving applications from having to do this on their own (or,
in fact, do any special handling to receive multicast packets at all).

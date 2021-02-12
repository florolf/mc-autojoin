#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>

#define error(format, ...) do { fprintf(stderr, format "\n", ##__VA_ARGS__); } while(0)
#define die(format, ...) do { error(format, ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)
#define pdie(string, ...) do { perror(string); exit(EXIT_FAILURE); } while(0)

static int verbose = 0;
static int strict = 0;

static int sock;
static struct in_addr *addrs = NULL;

static int subscribe_all(int ifindex)
{
	struct ip_mreqn req = {
		// kernel (rightfully) ignores imr_address when ifindex is set
		.imr_ifindex = ifindex
	};

	for (struct in_addr *addr = addrs; addr->s_addr; addr++) {
		memcpy(&req.imr_multiaddr, addr, sizeof(*addr));

		if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(req)) < 0) {
			if (errno == EADDRINUSE) {
				error("redundant subscription to %s on %d", inet_ntoa(*addr), ifindex);
				continue;
			}

			int tmp_errno = errno;
			error("subscribing to %s failed: %s", inet_ntoa(*addr), strerror(tmp_errno));

			if (strict)
				die("subscription failed in strict mode");
		}
	}

	return 0;
}

static void cache_changed(struct nl_cache *cache, struct nl_object *obj, int action, void *arg)
{
	(void) cache;
	(void) arg;

	struct rtnl_link *link = (struct rtnl_link*) obj;

	if (action != NL_ACT_NEW)
		return;

	int ifindex = rtnl_link_get_ifindex(link);
	if (verbose)
		printf("new link: %s / %d\n", rtnl_link_get_name(link), ifindex);

	if (subscribe_all(ifindex))
		die("subscription failed on %s / %d", rtnl_link_get_name(link), ifindex);
}

static void cache_iter(struct nl_object *obj, void *arg)
{
	(void) arg;

	struct rtnl_link *link = (struct rtnl_link*) obj;

	int ifindex = rtnl_link_get_ifindex(link);
	if (verbose)
		printf("init: %s / %d\n", rtnl_link_get_name(link), ifindex);

	if (subscribe_all(ifindex))
		die("subscription failed on %s / %d", rtnl_link_get_name(link), ifindex);
}

static int track_interfaces(void)
{
	int err;

	struct nl_sock *sk;
	sk = nl_socket_alloc();
	if (!sk)
		die("could not open netlink socket");

	struct nl_cache_mngr *mngr;
	err = nl_cache_mngr_alloc(sk, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);
	if (err < 0) {
		nl_perror(err, "allocating cache manager failed");
		return EXIT_FAILURE;
	}

	struct nl_cache *cache;
	err = nl_cache_mngr_add(mngr, "route/link", cache_changed, NULL, &cache);
	if (err < 0) {
		nl_perror(err, "subscribing to link updates failed");
		return EXIT_FAILURE;
	}

	nl_cache_foreach(cache, cache_iter, NULL);

	while (1) {
		err = nl_cache_mngr_poll(mngr, -1);
		if (err < 0 && err != NLE_INTR) {
			nl_perror(err, "nl_cache_mngr_poll()");
			return -1;
		}
	}
}

static void usage(const char *name, int ret)
{
	printf("usage: %s [-vs] addr1 [addr2 ...]\n", name);
	exit(ret);
}

int main(int argc, char **argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "vsh")) != -1) {
		switch (opt) {
			case 'v':
				verbose++;
				break;
			case 's':
				strict = 1;
				break;
			case 'h':
				usage(argv[0], EXIT_SUCCESS);
				break;
			default:
				usage(argv[0], EXIT_FAILURE);
				break;
		}
	}


	if (argc == optind)
		usage(argv[0], EXIT_FAILURE);

	argc -= optind;
	argv += optind;

	addrs = calloc(argc + 1, sizeof(*addrs));
	if (!addrs)
		pdie("failed to allocate address list");

	for (int i = 0; i < argc; i++) {
		if (inet_aton(argv[i], &addrs[i]) == 0)
			die("failed to parse address '%s'", argv[i]);
	}

	addrs[argc].s_addr = 0;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		pdie("could not create dummy socket");

	if (track_interfaces() < 0)
		die("failed to track interfaces");

	return EXIT_FAILURE;
}

# CAP_NET_BIND_SERVICE

```c:net/ipv4/af_inet.c
int __inet_bind(struct sock *sk, struct sockaddr *uaddr, int addr_len,
  u32 flags)
{
 struct sockaddr_in *addr = (struct sockaddr_in *)uaddr;
 struct inet_sock *inet = inet_sk(sk);
 struct net *net = sock_net(sk);
 unsigned short snum;
 int chk_addr_ret;
 u32 tb_id = RT_TABLE_LOCAL;
 int err;

 if (addr->sin_family != AF_INET) {
  /* Compatibility games : accept AF_UNSPEC (mapped to AF_INET)
   * only if s_addr is INADDR_ANY.
   */
  err = -EAFNOSUPPORT;
  if (addr->sin_family != AF_UNSPEC ||
      addr->sin_addr.s_addr != htonl(INADDR_ANY))
   goto out;
 }

 tb_id = l3mdev_fib_table_by_index(net, sk->sk_bound_dev_if) ? : tb_id;
 chk_addr_ret = inet_addr_type_table(net, addr->sin_addr.s_addr, tb_id);

 /* Not specified by any standard per-se, however it breaks too
  * many applications when removed.  It is unfortunate since
  * allowing applications to make a non-local bind solves
  * several problems with systems using dynamic addressing.
  * (ie. your servers still start up even if your ISDN link
  *  is temporarily down)
  */
 err = -EADDRNOTAVAIL;
 if (!inet_addr_valid_or_nonlocal(net, inet, addr->sin_addr.s_addr,
                                  chk_addr_ret))
  goto out;

 snum = ntohs(addr->sin_port);
 err = -EACCES;
 if (!(flags & BIND_NO_CAP_NET_BIND_SERVICE) &&
     snum && inet_port_requires_bind_service(net, snum) &&
     !ns_capable(net->user_ns, CAP_NET_BIND_SERVICE))
  goto out;

 /*      We keep a pair of addresses. rcv_saddr is the one
  *      used by hash lookups, and saddr is used for transmit.
  *
  *      In the BSD API these are the same except where it
  *      would be illegal to use them (multicast/broadcast) in
  *      which case the sending device address is used.
  */
 if (flags & BIND_WITH_LOCK)
  lock_sock(sk);

 /* Check these errors (active socket, double bind). */
 err = -EINVAL;
 if (sk->sk_state != TCP_CLOSE || inet->inet_num)
  goto out_release_sock;

 inet->inet_rcv_saddr = inet->inet_saddr = addr->sin_addr.s_addr;
 if (chk_addr_ret == RTN_MULTICAST || chk_addr_ret == RTN_BROADCAST)
  inet->inet_saddr = 0;  /* Use device */

 /* Make sure we are allowed to bind here. */
 if (snum || !(inet->bind_address_no_port ||
        (flags & BIND_FORCE_ADDRESS_NO_PORT))) {
  err = sk->sk_prot->get_port(sk, snum);
  if (err) {
   inet->inet_saddr = inet->inet_rcv_saddr = 0;
   goto out_release_sock;
  }
  if (!(flags & BIND_FROM_BPF)) {
   err = BPF_CGROUP_RUN_PROG_INET4_POST_BIND(sk);
   if (err) {
    inet->inet_saddr = inet->inet_rcv_saddr = 0;
    if (sk->sk_prot->put_port)
     sk->sk_prot->put_port(sk);
    goto out_release_sock;
   }
  }
 }

 if (inet->inet_rcv_saddr)
  sk->sk_userlocks |= SOCK_BINDADDR_LOCK;
 if (snum)
  sk->sk_userlocks |= SOCK_BINDPORT_LOCK;
 inet->inet_sport = htons(inet->inet_num);
 inet->inet_daddr = 0;
 inet->inet_dport = 0;
 sk_dst_reset(sk);
 err = 0;
out_release_sock:
 if (flags & BIND_WITH_LOCK)
  release_sock(sk);
out:
 return err;
}
```

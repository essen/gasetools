-module(psu_patch).
-export([start/0]).

% patch: 202.51.6.31
-define(PATCH_HOST, "patchpc.ipsu.segaonline.jp").
-define(PATCH_PORT, 11030).

% patch download: IP sent by patch server
-define(DL_HOST, "202.51.6.32").
-define(DL_PORT, 11031).

-define(TCP_OPTIONS, [binary, {packet, 0}, {active, false}, {reuseaddr, true}]).

start() ->
	spawn(fun dl_listen/0),
	patch_listen().

patch_listen() ->
	{ok, LSocket} = gen_tcp:listen(?PATCH_PORT, ?TCP_OPTIONS),
	patch_accept(LSocket).

patch_accept(LSocket) ->
	{ok, CSocket} = gen_tcp:accept(LSocket),
	patch_proxy_client(CSocket),
	patch_accept(LSocket).

% patch_proxy_client

patch_proxy_client(CSocket) ->
	{ok, SSocket} = gen_tcp:connect(?PATCH_HOST, ?PATCH_PORT, ?TCP_OPTIONS),
	{ok, ServerState, ClientState} = patch_proxy_hello(SSocket, CSocket, "patch-server"),
	spawn(fun() -> patch_proxy(CSocket, SSocket, ClientState, 56, "patch-client", 1) end),
	spawn(fun() -> patch_spoof_dl(SSocket, CSocket, ServerState) end).

% patch_proxy_hello

patch_proxy_hello(SocketRecv, SocketSend, Name) ->
	io:format("hello packet!~n"),
	{ok, Packet} = gen_tcp:recv(SocketRecv, 0),
	file:write_file(lists:concat([Name, "-packet-0.bin"]), Packet),
	gen_tcp:send(SocketSend, Packet),
	<<_:96, % packet size:32, command:16, parameter:16, unknown but required:32
	  ServerSeed:32/little-unsigned-integer,
	  ClientSeed:32/little-unsigned-integer,
	  _/bits>> = Packet, % client cipher flag:8, server cipher flag:8, probably padding:16, unknown:32, probably padding:96
	{ok, cipher_init(ServerSeed), cipher_init(ClientSeed)}.

% patch_proxy

patch_proxy(SocketRecv, SocketSend, State, Acc, Name, Number) ->
	case gen_tcp:recv(SocketRecv, 0) of
		{ok, Packet} ->
			io:format("handling patch packet: ~s-~.10B~n", [Name, Number]),
			{DecryptedPacket, RState, RAcc} = cipher(Packet, State, Acc),
			file:write_file(lists:concat([Name, "-packet-", Number, ".bin"]), DecryptedPacket),
			gen_tcp:send(SocketSend, Packet),
			patch_proxy(SocketRecv, SocketSend, RState, RAcc, Name, Number + 1);
		{error, closed} ->
			io:format("error receiving packet~n")
	end.

% patch_spoof_dl: get the server packet giving the dl server IP and replace it with the proxy IP

patch_spoof_dl(SSocket, CSocket, State) ->
	io:format("spoof server packet!~n"),
	{ok, Packet} = gen_tcp:recv(SSocket, 0),
	{DecryptedPacket, _, _} = cipher(Packet, State, 56),
	file:write_file("patch-server-packet-1.bin", DecryptedPacket),
	<<Before:64/bits, _:32, After:32/bits>> = DecryptedPacket,
	SpoofedPacket = <<Before/bits, 192, 168, 1, 15, After/bits>>,
	file:write_file("patch-server-packet-1-spoofed.bin", SpoofedPacket),
	{EncryptedPacket, _, _} = cipher(SpoofedPacket, State, 56),
	gen_tcp:send(CSocket, EncryptedPacket).

% dl_listen

dl_listen() ->
	{ok, LSocket} = gen_tcp:listen(?DL_PORT, ?TCP_OPTIONS),
	dl_accept(LSocket).

% dl_accept: just proxy directly for now

dl_accept(LSocket) ->
	{ok, CSocket} = gen_tcp:accept(LSocket),
	{ok, SSocket} = gen_tcp:connect(?DL_HOST, ?DL_PORT, ?TCP_OPTIONS),
	{ok, ServerState, ClientState} = patch_proxy_hello(SSocket, CSocket, "dl-server"),
	spawn(fun() -> dl_proxy(CSocket, SSocket, ClientState, 56, "dl-client", 1) end),
	spawn(fun() -> dl_proxy(SSocket, CSocket, ServerState, 56, "dl-server", 1) end),
	dl_accept(LSocket).

% dl_proxy

dl_proxy(SocketRecv, SocketSend, State, Acc, Name, Number) ->
	case gen_tcp:recv(SocketRecv, 0) of
		{ok, Packet} ->
			io:format("handling patch packet: ~s-~.10B~n", [Name, Number]),
			{DecryptedPacket, RState, RAcc} = cipher(Packet, State, Acc),
			file:write_file(lists:concat([Name, "-packet-", Number, ".bin"]), DecryptedPacket),
			gen_tcp:send(SocketSend, Packet),
			dl_proxy(SocketRecv, SocketSend, RState, RAcc, Name, Number + 1);
		{error, closed} ->
			io:format("error receiving packet~n")
	end.

% cipher_init

cipher_init(Seed) ->
	State = array:set(56, Seed, array:set(55, Seed, array:new(57))),
	cipher_shuffle(cipher_shuffle(cipher_shuffle(cipher_shuffle(cipher_init(Seed, 1, 21, State))))).

cipher_init(Seed, Next, Count, State) when Count =< 1134 ->
	cipher_init(Next, cipher_32bit_sub(Seed, Next), Count + 21, array:set(Count rem 55, Next, State));
cipher_init(_, _, _, State) ->
	State.

% cipher_shuffle

cipher_shuffle(State) ->
	cipher_shuffle(cipher_shuffle(State, 1, 24, 31), 25, 31, -24).

cipher_shuffle(State, Index, Count, Inc) when Count > 0 ->
	cipher_shuffle(array:set(Index, cipher_32bit_sub(array:get(Index, State), array:get(Index + Inc, State)), State), Index + 1, Count - 1, Inc);
cipher_shuffle(State, _, _, _) ->
	State.

% cipher_32bit_sub (32bit round sub)

cipher_32bit_sub(A, B) when B > A ->
	16#100000000 + A - B;
cipher_32bit_sub(A, B) ->
	A - B.

% cipher

cipher(Data, State, Acc) when Acc == 56 ->
	cipher(Data, cipher_shuffle(State), 1);
cipher(<<Value:32/little-unsigned-integer, Rest/bits>>, State, Acc) ->
	{RData, _, RAcc} = cipher(Rest, State, Acc + 1),
	{<<(Value bxor array:get(Acc, State)):32/little-unsigned-integer, (RData)/bits>>, State, RAcc};
cipher(_, State, Acc) ->
	{<<>>, State, Acc}.

-module(psu_proxy).
-export([start/0]).

% start with: sudo erl -ssl protocol_version '[sslv3]'

% login: 202.51.6.30
-define(LOGIN_HOST, "loginpc.ipsu.segaonline.jp").
-define(LOGIN_PORT, 12030).

-define(LISTEN_OPTIONS, [binary, {active, false}, {reuseaddr, true}, {ssl_imp, new}, {certfile, "servercert.pem"}, {keyfile, "serverkey.pem"}, {password, "alpha"}]).
-define(CONNECT_OPTIONS, [binary, {active, false}, {reuseaddr, true}, {ssl_imp, new}, {certfile, "clientcert.pem"}, {keyfile, "clientkey.pem"}, {password, "alpha"}]).

start() ->
	% ssl
	application:start(crypto),
	application:start(public_key),
	application:start(ssl),
	ssl:seed("Suits up!"), % TODO: should be random
	% start
	login_listen().

login_listen() ->
	{ok, LSocket} = ssl:listen(?LOGIN_PORT, ?LISTEN_OPTIONS),
	login_accept(LSocket).

login_accept(LSocket) ->
	{ok, CSocket} = ssl:transport_accept(LSocket),
	ok = ssl:ssl_accept(CSocket),
	{ok, SSocket} = ssl:connect(?LOGIN_HOST, ?LOGIN_PORT, ?CONNECT_OPTIONS),
	spawn(fun() -> login_proxy(CSocket, SSocket, "login-client", 1) end),
	spawn(fun() -> login_proxy(SSocket, CSocket, "login-server", 1) end),
	login_accept(LSocket).

login_spoof_game(SocketSend, Packet) ->
	io:format("spoofing game server IP~n"),
	<<Before:352/bits, GameIP:32/bits, GamePort:16/little-unsigned-integer, After/bits>> = Packet,
	% TODO: game IP is still hardcoded
	SpoofedPacket = <<Before/bits, 192, 168, 1, 42, GamePort:16/little-unsigned-integer, After/bits>>,
	spawn(fun() -> game_listen(GameIP, GamePort) end),
	ssl:send(SocketSend, SpoofedPacket).

login_proxy(SocketRecv, SocketSend, Name, Number) ->
	case ssl:recv(SocketRecv, 0, 50) of
		{ok, Packet} ->
			<<_:32, Command:24/unsigned-integer, _/bits>> = Packet,
			io:format("handling login packet: ~s-~.10B~n", [Name, Number]),
			file:write_file(lists:concat([Name, "-packet-", Number, ".bin"]), Packet),
			case Command of
				16#021603 ->
					login_spoof_game(SocketSend, Packet);
				_ ->
					ssl:send(SocketSend, Packet),
					login_proxy(SocketRecv, SocketSend, Name, Number + 1)
			end;
		{error, closed} ->
			io:format("socket ~s closed~n", [Name]),
			ssl:close(SocketSend); % close other socket
		{error, timeout} ->
			login_proxy(SocketRecv, SocketSend, Name, Number)
	end.

game_listen(GameIP, GamePort) ->
	{ok, LSocket} = ssl:listen(GamePort, ?LISTEN_OPTIONS),
	game_accept(LSocket, GameIP, GamePort).

game_accept(LSocket, GameIP, GamePort) ->
	{ok, CSocket} = ssl:transport_accept(LSocket),
	ok = ssl:ssl_accept(CSocket),
	<<A:8/little-unsigned-integer, B:8/little-unsigned-integer, C:8/little-unsigned-integer, D:8/little-unsigned-integer>> = GameIP,
	{ok, SSocket} = ssl:connect({A, B, C, D}, GamePort, ?CONNECT_OPTIONS),
	io:format("game server ~.10B.~.10B.~.10B.~.10B:~.10B connected!~n", [A, B, C, D, GamePort]),
	spawn(fun() -> game_proxy(CSocket, SSocket, << 16#43, 16#4c, 16#4e, 16#54 >>, 1) end),
	spawn(fun() -> game_proxy(SSocket, CSocket, << 16#53, 16#45, 16#52, 16#56 >>, 1) end),
	game_accept(LSocket, GameIP, GamePort).

game_proxy(SocketRecv, SocketSend, Name, Number) ->
	case ssl:recv(SocketRecv, 0, 50) of
		{ok, Packet} ->
			io:format("handling game packet: ~s-~.10B~n", [Name, Number]),
			file:write_file("log.bin", << Name/binary, Number:32/little-unsigned-integer, Packet/binary >>, [append]),
			ssl:send(SocketSend, Packet),
			game_proxy(SocketRecv, SocketSend, Name, Number + 1);
		{error, closed} ->
			io:format("socket ~s closed~n", [Name]),
			ssl:close(SocketSend); % close other socket
		{error, timeout} ->
			game_proxy(SocketRecv, SocketSend, Name, Number)
	end.

function negotiate (envelope, f0, f1, f2, f3)
  print("[" .. envelope.sid .. "] negotiate")
  r = Milter.setsymlist(envelope, Milter.SMFIM_HELO, "{client_addr}");
  print("setsymlist: " .. r)
  return Milter.SMFIS_ALL_OPTS
end

function helo (envelope)
  print("[" .. envelope.sid .. "] helo")
  return Milter.SMFIS_CONTINUE
end

function close (envelope)
  print("[" .. envelope.sid .. "] close")
  return Milter.SMFIS_CONTINUE
end

milter = Milter.create()
milter:setConnection("inet:12345")
milter:setFlags(0)
milter:setNegotiate(negotiate)
milter:setHELO(helo)
milter:setClose(close)
print(Milter.SMFIS_CONTINUE, "continue")
print(Milter.SMFIS_REJECT, "reject")
print(Milter.SMFIS_DISCARD, "discard")
print(Milter.SMFIS_ACCEPT, "accept")
print(Milter.SMFIS_TEMPFAIL, "tempfail")
print(Milter.SMFIS_NOREPLY, "noreply")
print(Milter.SMFIS_SKIP, "skip")
return milter

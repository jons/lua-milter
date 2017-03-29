

function helo (envelope)
  print("helo")
  print(Milter.SMFIS_CONTINUE, "continue")
  print(Milter.SMFIS_REJECT, "reject")
  print(Milter.SMFIS_DISCARD, "discard")
  print(Milter.SMFIS_ACCEPT, "accept")
  print(Milter.SMFIS_TEMPFAIL, "tempfail")
  print(Milter.SMFIS_NOREPLY, "noreply")
  print(Milter.SMFIS_SKIP, "skip")
  return 0
end

obj = Milter.start("inet:12345", 0)

print(obj)

print(obj:connstr())

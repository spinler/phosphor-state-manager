# Redundant BMC Management

The phosphor-rbmc-state-manager application manages redundant BMC functionality.

It hosts an xyz.openbmc_project.State.BMC.Redundancy interface on a service of
the same name, on the /xyz/openbmc_project/state/bmc0 object path. This
interface provides the Active vs Passive role property as well as some other
redundancy related properties.

## Startup

So far on startup the application will just immediately determine its role.

## Role Determination Rules

The current rules for role determination are:

1. If BMC position zero choose the active role, otherwise passive.

If there is an internal failure during role determination, like an exception,
the BMC will also have to become passive.

## After the role is determined

After the role has been determined, the code will:

1. Update the Role property on D-Bus

The active BMC will also:

1. Start the obmc-bmc-active.target systemd target to start any service that
   only run on the active BMC.

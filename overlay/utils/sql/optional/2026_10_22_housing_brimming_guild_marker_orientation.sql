-- Original OBP_GUILDSQUARE mesh reviewed with exact native rotation matrices.
-- Face the valley ramp; compensate the pivot to preserve the world XY bounds.
-- Legal region, prices, ownership and placed-item transforms are unchanged.
-- Idempotent: only the known old reconstructed marker transform is eligible.
UPDATE doors SET pos_x=pos_x-2.96590555516104,pos_y=pos_y+9.83795808445734,heading=MOD(heading+256,512)
WHERE zone='neighborhood' AND version=0 AND doorid=202 AND name='OBP_GUILDSQUARE'
 AND size=100 AND opentype=160 AND ABS(pos_x-571.65)<0.01 AND ABS(pos_y+2227.774825)<0.01 AND ABS(pos_z)<0.01 AND ABS(heading-508.940979004)<0.01;

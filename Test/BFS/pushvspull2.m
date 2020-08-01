function pushvspull (name, allpush_results, allpull_results, n, nz, fig)
% pushvspull ('kron', allpush_results, allpull_results, n, nz, 1) ;

d = nz / n 
ntrials = length (allpush_results) ;
format long g
figure (fig)
clf (fig)

fprintf ('\n--------------- %s:\n', name) ;

t_pull_tot = 0 ;
t_push_tot = 0 ;
t_auto_tot = 0 ;
t_best_tot = 0 ;

for k = 1:ntrials

    push_result = allpush_results {k}  ;
    pull_result = allpull_results {k}  ;

    edges_unexplored = nz ;
    last_nq = 0 ;
    do_push = true ;

    nq = push_result (:,2)
    nvisited = push_result (:,3)
    edges_in_frontier = push_result (:,4)
    t_pull = pull_result (:,5)
    t_push = push_result (:,5)

    reltime = t_push ./ t_pull ;

    tt = [t_pull t_push]  ;
    t_best = min (tt, [ ], 2) ;
    nlevels = size (push_result, 1) ;
    t_auto = nan (nlevels, 1) ;
    edges_unexploreds = nan (nlevels, 1) ;
    growings = nan (nlevels, 1) ;

    for level = 1:nlevels
        % select push or pull
        this_nq = nq (level) ;
        edges_unexplored = edges_unexplored - edges_in_frontier (level) ;
        edges_unexploreds (level) = edges_unexplored ;
        growing = this_nq > last_nq ;
        growings (level) = growing ;
        if (do_push)
            big_frontier = edges_in_frontier (level) > (edges_unexplored / 15) ;
            if (big_frontier && growing)
                do_push = false ;
            end
        else
            shrinking = ~growing ;
            if ((this_nq < n / 18) & shrinking)
                do_push = true ;
            end
        end
        if (do_push)
            t_auto (level) = t_push (level);
        else
            t_auto (level) = t_pull (level);
        end
        bad_choice = (t_auto (level) > t_best (level)) ;
        if (bad_choice)
            fprintf ('   level %2d: push %10.4f pull %10.3f ', ...
                level, t_push (level), t_pull (level)) ;
            if (do_push)
                fprintf ('      ');
            else
                fprintf (' pull ');
            end
            if (bad_choice)
                fprintf (' (oops)') ;
            end
            fprintf ('\n') ;
        end
        last_nq = this_nq ;
    end

    fprintf ('trial %2d : ', k) ;
    fprintf ('push %10.4f ', sum (t_push)) ;
    fprintf ('pull %10.4f ', sum (t_pull)) ;
    fprintf ('auto %10.4f ', sum (t_auto)) ;
    fprintf ('best %10.4f ', sum (t_best)) ;
    fprintf ('(%10.3f) ', sum (t_auto) / sum (t_push)) ;
    fprintf ('(%10.3f)\n', sum (t_auto) / sum (t_best)) ;

    t_pull_tot = t_pull_tot + sum (t_pull) ;
    t_push_tot = t_push_tot + sum (t_push) ;
    t_auto_tot = t_auto_tot + sum (t_auto) ;
    t_best_tot = t_best_tot + sum (t_best) ;

    relalpha = (edges_unexploreds) ./ (edges_in_frontier) ;

    relalpha
    reltime
    growings

    % subplot (2,2,1)
    loglog ( relalpha (growings ~= 0), reltime (growings ~= 0), 'o') ;
%            [1e-8 100], [1 1], 'g-', ...
%            [1 1], [1e-3 10], 'g-') ;
    ylabel ('pushtime / pulltime') ;
    xlabel ('alpha') ;
    title (name) ;
    hold on
    drawnow

%{
    subplot (2,2,2)
    loglog ( pushwork, t_push, 'o') ;
    ylabel ('push time') ;
    xlabel ('push work') ;
    hold on

    subplot (2,2,3)
    loglog ( pullwork, t_pull, 'o') ;
    ylabel ('pull time') ;
    xlabel ('pull work') ;
    hold on
%}

%   subplot (2,2,2)
%   semilogy ( nvisited/n, reltime, 'o') ;
%   ylabel ('nvisited/n') ;
%   ylabel ('pushtime / pulltime') ;

%   subplot (2,2,3)
%   x = (nvisited -nq) / n; % ./(nvisited-nq) ;
%   semilogy ( x, t_push, 'o', x, t_pull, 'rx') ;

%   subplot (1,2,2)
%   loglog ( ...
%       relwork (:), t_push (:), 'o', ...
%       relwork (:), t_pull (:), 'rx', ...
%            [1 1], [1e-3 10], 'g-') ;

end
hold off

fprintf ('all trials: allpush: %10.3f allpull: %10.3f auto: %10.3f best: %10.3f\n', ...
    t_push_tot, t_pull_tot, t_auto_tot, t_best_tot) ;

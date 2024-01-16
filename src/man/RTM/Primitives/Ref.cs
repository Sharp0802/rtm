namespace RTM.Primitives;

public unsafe class Ref<T>(T* p) where T : unmanaged
{
    private readonly T* _p = p;

    public T Dereference() => *_p;

    public static implicit operator Ref<T>(T* from) => new(from);

    public static implicit operator T*(Ref<T> from) => from._p;
}